#include <map>
#include <pin.H>
#include <vector>
#include <string>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern "C" {
#include <xed-interface.h>
}

#include "lbr.h"

size_t lbr_stack_size = 16;
bool kbouncer_checks_enabled = true;
std::map<VOID*, std::string> func_name_map;
size_t GADGET_INS_LENGTH_THRESHOLD = 20;
size_t GADGET_CHAIN_LENGTH_THRESHOLD = 8;

// TODO: create mapping for LBR per thread
LBR* test_lbr = NULL;

KNOB<string> enable_checks_flag_arg(
		KNOB_MODE_WRITEONCE,
		"pintool",
		"enable_checks",
		"true",
		"Determines if kbouncer checks should be active (default: true)");

KNOB<string> ins_limit_arg(
		KNOB_MODE_WRITEONCE,
		"pintool",
		"ins_limit",
		"20",
		"Upper bound on the number of gadget instructions (default: 20)");

KNOB<string> chain_limit_arg(
		KNOB_MODE_WRITEONCE,
		"pintool",
		"chain_min",
		"8",
		"Lower bound on the number of consecutive gadget-like blocks for ROP (default: 8)");

KNOB<string> lbr_size_arg(
		KNOB_MODE_WRITEONCE,
		"pintool",
		"lbr_size",
		"16",
		"Size of LBR stack (default: 16)");

bool IsControlFlowTransferInstruction(const char* inst_buf) {
	// TODO: Find a better way to do this during at analysis time
	// We use some heuristics to cut down on the number of searches
	// Since there are a million jumps in x86 we just search for j
	// since only jumps have j in their opcode string
	if(strstr(inst_buf, "ret")
			|| strstr(inst_buf, "j")
			|| strstr(inst_buf, "call")
			|| strstr(inst_buf, "enter")
			|| strstr(inst_buf, "int")
			|| strstr(inst_buf, "into")
			|| strstr(inst_buf, "lcall")
			|| strstr(inst_buf, "loop")
			|| strstr(inst_buf, "bound")) {
		return true;
	} else {
		return false;
	}
}

bool InsHasMemoryRead(const xed_decoded_inst_t* xed_inst, size_t no_ops) {
	for(size_t i = 0; i < no_ops; i++) {
		if(xed_decoded_inst_mem_read(xed_inst, i)) return true;
	}
	return false;
}

VOID PrintInstUntilIndirectJump(const VOID* pc) {
	char inst_buf[2048];
	uint8_t inst_bytes[15];
	xed_decoded_inst_t xed_inst;
	xed_error_enum_t ret_code;
	xed_uint64_t runtime_addr;
	size_t inst_length = 0;
	xed_state_t dstate;
	size_t operand_count = 0;
	bool is_memory_read = false;
	size_t inst_count = 0;
	dstate.mmode = XED_MACHINE_MODE_LONG_64;
	dstate.stack_addr_width = XED_ADDRESS_WIDTH_64b;
	ADDRINT pc_from_ptr = (ADDRINT)pc;
	while(true) {
		xed_decoded_inst_zero_set_mode(&xed_inst, &dstate);
		PIN_SafeCopy(inst_bytes, (VOID*)pc_from_ptr, XED_MAX_INSTRUCTION_BYTES);
		ret_code = xed_decode(&xed_inst, inst_bytes, XED_MAX_INSTRUCTION_BYTES);
		if(ret_code == XED_ERROR_NONE) {
			inst_count += 1;
			operand_count = xed_decoded_inst_noperands(&xed_inst);
			is_memory_read = InsHasMemoryRead(&xed_inst, operand_count);
			runtime_addr = static_cast<xed_uint64_t>(pc_from_ptr);
			xed_format_context(
					XED_SYNTAX_INTEL,
					&xed_inst,
					inst_buf, 2048, runtime_addr, NULL, NULL
					);
			inst_length = xed_decoded_inst_get_length(&xed_inst);
			fprintf(stdout,
					"[%lu] %p [INS]: %s | length: %lu | # operands: %lu | memory is read: %s\n",
					inst_count, (void*)pc_from_ptr, inst_buf, inst_length,
					operand_count, is_memory_read ? "yes" : "no"
					);
			if(IsControlFlowTransferInstruction(inst_buf) && is_memory_read) {
				break;
			} else if(inst_count >= GADGET_INS_LENGTH_THRESHOLD) {
				fprintf(stdout,
						"Stopping INS walk at PC %p; no indirect jump found so far..\n",
						(void*)pc_from_ptr);
				break;
			} else {
				pc_from_ptr += inst_length;
			}
		} else {
			fprintf(stdout,
					"Failed to disassemble instruction at address %p | Error: %d\n",
					(void*)pc_from_ptr, ret_code
					);
			break;
		}
		memset(inst_buf, 0, 2048);
	    memset(inst_bytes, 0, 15);
	}
}

bool IsCallPrecededInstruction(VOID* dest) {
	uint8_t inst_bytes[15];
	uint8_t* bytes_end = &inst_bytes[14];
	xed_decoded_inst_t xed_inst;
	xed_error_enum_t ret_code;
	xed_uint64_t runtime_addr;
	xed_state_t dstate;
	char inst_buf[2048];
	size_t inst_size = 0;
	bool found_call_instruction = false;
	ADDRINT pc_from_ptr = (ADDRINT)dest; // here we are looking at the previous bytes
	PIN_LockClient();
	std::string dst_rtn_name;
	RTN dst_rtn = RTN_FindByAddress((ADDRINT)dest);
	if(RTN_Valid(dst_rtn)) {
		dst_rtn_name = RTN_Name(dst_rtn);
	}
	PIN_SafeCopy(inst_bytes, (VOID*)(pc_from_ptr - 15), XED_MAX_INSTRUCTION_BYTES);
	pc_from_ptr -= 1;
	for(uint8_t* bytes = bytes_end; bytes >= inst_bytes; bytes--, pc_from_ptr--) {
		xed_decoded_inst_zero_set_mode(&xed_inst, &dstate);
		inst_size = (size_t)(bytes_end - bytes) + 1;
		ret_code = xed_decode(&xed_inst, bytes, inst_size);
		if(ret_code == XED_ERROR_NONE) {
			runtime_addr = static_cast<xed_uint64_t>(pc_from_ptr);
			xed_format_context(
					XED_SYNTAX_INTEL,
					&xed_inst,
					inst_buf, 2048, runtime_addr, NULL, NULL
					);
			//if(!RTN_Valid(src_rtn) || !RTN_Valid(dst_rtn)) continue;
			/*fprintf(stdout,
					"ret to [%p]-[%s] possible preceding inst: %s [%p] | %lu bytes checked\n",
					dest, dst_rtn_name.c_str(),
					inst_buf, (VOID*)pc_from_ptr, inst_size);*/
			if(strstr(inst_buf, "call") != NULL) {
				found_call_instruction = true;
				break;
			}
			memset(inst_buf, 0, 2048);
		}
	}
	PIN_UnlockClient();
	//fprintf(stdout, "\n");
	return found_call_instruction;
}

size_t GetNumberOfInstructionsBetween(VOID* branch, VOID* target) {
	uint8_t inst_bytes[15];
	xed_decoded_inst_t xed_inst;
	xed_error_enum_t ret_code;
	size_t inst_length = 0;
	xed_state_t dstate;
	size_t ins_count = 0;
	dstate.mmode = XED_MACHINE_MODE_LONG_64;
	dstate.stack_addr_width = XED_ADDRESS_WIDTH_64b;
	ADDRINT pc_from_ptr = (ADDRINT)branch;
	while(pc_from_ptr <= (ADDRINT)target) {
		xed_decoded_inst_zero_set_mode(&xed_inst, &dstate);
		PIN_SafeCopy(inst_bytes, (VOID*)pc_from_ptr, XED_MAX_INSTRUCTION_BYTES);
		ret_code = xed_ild_decode(&xed_inst, inst_bytes, XED_MAX_INSTRUCTION_BYTES);
		if(ret_code == XED_ERROR_NONE) {
			inst_length = xed_decoded_inst_get_length(&xed_inst);
			pc_from_ptr += inst_length;
			ins_count += 1;
		} else {
			fprintf(stdout,
					"Failed to disassemble instruction at address %p | Error: %d\n",
					(void*)pc_from_ptr, ret_code
					);
			break;
		}
	    memset(inst_bytes, 0, 15);
	}
	return ins_count;
}

bool PrecedesIndirectJumpWithinThreshold(VOID* start_addr, VOID* jmp_addr) {
	char inst_buf[2048];
	uint8_t inst_bytes[15];
	xed_decoded_inst_t xed_inst;
	xed_error_enum_t ret_code;
	xed_uint64_t runtime_addr;
	size_t inst_length = 0;
	xed_state_t dstate;
	size_t inst_count = 0;
	size_t operand_count = 0;
	bool is_memory_read = false;
	dstate.mmode = XED_MACHINE_MODE_LONG_64;
	dstate.stack_addr_width = XED_ADDRESS_WIDTH_64b;
	ADDRINT pc_from_ptr = (ADDRINT)start_addr;
	while(true) {
		xed_decoded_inst_zero_set_mode(&xed_inst, &dstate);
		PIN_SafeCopy(inst_bytes, (VOID*)pc_from_ptr, XED_MAX_INSTRUCTION_BYTES);
		ret_code = xed_decode(&xed_inst, inst_bytes, XED_MAX_INSTRUCTION_BYTES);
		operand_count = xed_decoded_inst_noperands(&xed_inst);
		is_memory_read = InsHasMemoryRead(&xed_inst, operand_count);
		if(ret_code == XED_ERROR_NONE) {
			runtime_addr = static_cast<xed_uint64_t>(pc_from_ptr);
			xed_format_context(
					XED_SYNTAX_INTEL,
					&xed_inst,
					inst_buf, 2048, runtime_addr, NULL, NULL
					);
			inst_length = xed_decoded_inst_get_length(&xed_inst);
			inst_count += 1;
			if((IsControlFlowTransferInstruction(inst_buf) && is_memory_read)
					|| inst_count >= GADGET_INS_LENGTH_THRESHOLD) {
				break;
			} else {
				pc_from_ptr += inst_length;
			}
		} else {
			fprintf(stdout,
					"Failed to disassemble instruction at address %p | Error: %d\n",
					(void*)pc_from_ptr, ret_code
					);
			break;
		}
		memset(inst_buf, 0, 2048);
	    memset(inst_bytes, 0, 15);
	}
	if((VOID*)pc_from_ptr == jmp_addr) {
		return inst_count <= GADGET_INS_LENGTH_THRESHOLD;
	} else {
		return false;
	}
}

bool IsReturnInstruction(VOID* pc) {
	char inst_buf[2048];
	uint8_t inst_bytes[15];
	xed_decoded_inst_t xed_inst;
	xed_error_enum_t ret_code;
	xed_uint64_t runtime_addr;
	xed_state_t dstate;
	dstate.mmode = XED_MACHINE_MODE_LONG_64;
	dstate.stack_addr_width = XED_ADDRESS_WIDTH_64b;
	xed_decoded_inst_zero_set_mode(&xed_inst, &dstate);
	PIN_SafeCopy(inst_bytes, pc, XED_MAX_INSTRUCTION_BYTES);
	ret_code = xed_decode(&xed_inst, inst_bytes, XED_MAX_INSTRUCTION_BYTES);
	if(ret_code == XED_ERROR_NONE) {
		runtime_addr = static_cast<xed_uint64_t>((ADDRINT)pc);
		xed_format_context(
				XED_SYNTAX_INTEL,
				&xed_inst,
				inst_buf, 2048,
				runtime_addr, NULL, NULL
				);
		return strstr(inst_buf, "ret") != NULL;
	} else {
		return false;
	}
}

size_t GetLongestDetectedGadgetSeqCount(const LBR* lbr) {
	if(lbr->GetStackSize() <= 0) return 0;
	size_t chain_count = 0;
	size_t max_chain_count = 0;
	size_t lbr_stack_size = lbr->GetStackSize();
	size_t i = lbr->GetTosPosition();
	size_t check_count = 0;
	VOID* branch = NULL;
	VOID* prev_branch_target = NULL;
	while(check_count < lbr_stack_size - 1) {
		branch = lbr->GetSrcAt(i);
		if(i == 0) {
			prev_branch_target = lbr->GetDstAt(lbr->GetStackSize() - 1);
		} else {
			prev_branch_target = lbr->GetDstAt(i - 1);
		}
		if(branch > prev_branch_target
		        && PrecedesIndirectJumpWithinThreshold(prev_branch_target, branch)) {
			chain_count += 1;
			fprintf(stdout,
					"Rb = %p | Rt(n - 1) = %p | Seems gadget-like..\n",
					branch, prev_branch_target);
			PrintInstUntilIndirectJump(prev_branch_target);
		} else {
			if(chain_count > 0) fprintf(stdout,
					"\n**** Chain reset | current len: %lu | max len so far: %lu ****\n\n",
					chain_count, max_chain_count);
			if(chain_count > max_chain_count) {
				max_chain_count = chain_count;
			}
			chain_count = 0;
		}
		check_count += 1;
		if(i == 0)
			i = lbr->GetStackSize() - 1;
		else
			i -= 1;
	}
	return max_chain_count; // callers can then check if chain_count >= 8
}



void PrintLbrStack(const LBR* lbr) {
    int stack_count = 0;
    fprintf(stdout, "\nLBR Stack\n");
    fprintf(stdout, "___________________\n");
    for(size_t i = 0; i < lbr->GetStackSize(); i++) {
        if (i == lbr->GetTosPosition()) {
            fprintf(stdout, "TOS=>");
        } else {
            fprintf(stdout, "     ");
        }
        fprintf(stdout, "[%d] Branch: %p (%s) | Target %p (%s)\n",
                stack_count + 1,
                lbr->GetSrcAt(i),
                func_name_map[lbr->GetSrcAt(i)].c_str(),
                lbr->GetDstAt(i),
                func_name_map[lbr->GetDstAt(i)].c_str());
        stack_count += 1;
    }
    fprintf(stdout, "___________________\n");
}


VOID AnalyzeOnIndirectBranch(VOID* src, VOID* dest) {
    RTN found_rtn;
    if(func_name_map.find(dest) == func_name_map.end()) {
        PIN_LockClient();
        found_rtn = RTN_FindByAddress((ADDRINT)dest);
        if(RTN_Valid(found_rtn)) {
            func_name_map[dest] = RTN_Name(found_rtn);
        }
        PIN_UnlockClient();
    }
    if(test_lbr) {
        test_lbr->AddBranchEntry(src, dest);
        //PrintInstUntilRet(dest);
        //PrintLbrStack(test_lbr);
    }
}


void InstrumentInstructions(INS ins, VOID* v) {
    RTN ins_rtn;
    if(INS_IsIndirectBranchOrCall(ins)) {
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE,
                    (AFUNPTR)AnalyzeOnIndirectBranch,
                    IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR,
                    IARG_END);
            ins_rtn = INS_Rtn(ins);
            if(RTN_Valid(ins_rtn)) {
                func_name_map[(VOID*)INS_Address(ins)] = RTN_Name(ins_rtn);
            }
    }
}

bool IllegalReturnsFoundInLBR(const LBR* lbr) {
	if(lbr->GetStackSize() <= 0) return false;
	bool illegal_ret_found = false;
	for(size_t i = 0; i < lbr->GetStackSize(); i++) {
		if(IsReturnInstruction(lbr->GetSrcAt(i)) &&
				!IsCallPrecededInstruction(lbr->GetDstAt(i))) {
			illegal_ret_found = true;
			fprintf(stdout,
					"Illegal ret %p -->  target %p\n",
					lbr->GetSrcAt(i), lbr->GetDstAt(i));
			PrintInstUntilIndirectJump(lbr->GetDstAt(i));
		}
	}
	return illegal_ret_found;
}

VOID CheckForRopBeforeSysCall(THREADID thread_index, CONTEXT* ctxt,
		SYSCALL_STANDARD std, VOID* v) {
	// We do as LBR walk to verify integrity of control flow
	if(!kbouncer_checks_enabled) return;
	fprintf(stdout,
			"\n[ Printing ROP diagnostics prior to syscall(%lu) ]\n\n",
			PIN_GetSyscallNumber(ctxt, std));
	PrintLbrStack(test_lbr);
	size_t chain_length = GetLongestDetectedGadgetSeqCount(test_lbr);
	if(IllegalReturnsFoundInLBR(test_lbr)) {
		fprintf(stdout, "ROP detected: Illegal return instruction found!\n");
	} else if(chain_length >= GADGET_CHAIN_LENGTH_THRESHOLD) {
		fprintf(stdout, "ROP detected: gadget chain of length %lu detected\n", chain_length);
	}
}


int main(int argc, char *argv[]) {
    // Initialize pin & symbol manager
    if (PIN_Init(argc, argv)) {
        return -1;
    }
    std::string enable_checks_str = enable_checks_flag_arg.Value();
    std::string ins_limit_str = ins_limit_arg.Value();
    std::string chain_limit_str = chain_limit_arg.Value();
    std::string lbr_size_str = lbr_size_arg.Value();
    if(strcasecmp(enable_checks_str.c_str(), "true") == 0) {
    	kbouncer_checks_enabled = true;
    } else {
    	kbouncer_checks_enabled = false;
    }
    int ins_limit_int = atoi(ins_limit_str.c_str());
    if(ins_limit_int > 0) GADGET_INS_LENGTH_THRESHOLD = ins_limit_int;
    int chain_limit_int = atoi(chain_limit_str.c_str());
    if(chain_limit_int > 0) GADGET_CHAIN_LENGTH_THRESHOLD = chain_limit_int;
    int lbr_size_int = atoi(lbr_size_str.c_str());
    if(lbr_size_int > 0) lbr_stack_size = lbr_size_int;
    fprintf(stdout,
    		"\nSimulation configuration\n"
    		"\tKbouncer checks enabled: %s\n"
    		"\tLBR stack size: %lu\n"
			"\tMax gadget instruction length: %lu\n"
			"\tMin consecutive gadget chain length: %lu\n",
			kbouncer_checks_enabled ? "YES" : "NO",
			lbr_stack_size,
			GADGET_INS_LENGTH_THRESHOLD,
			GADGET_CHAIN_LENGTH_THRESHOLD);
    PIN_InitSymbols();
    xed_tables_init();
    test_lbr = new LBR(lbr_stack_size);
    INS_AddInstrumentFunction(InstrumentInstructions, NULL);
    PIN_AddSyscallEntryFunction(CheckForRopBeforeSysCall, NULL);
    PIN_StartProgram();
    return 0;
}
