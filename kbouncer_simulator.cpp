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

std::map<VOID*, std::string> func_name_map;
const size_t GADGET_INS_LENGTH_THRESHOLD = 20;

// TODO: create mapping for LBR per thread
LBR* test_lbr = NULL;


VOID PrintInstUntilRet(VOID* pc) {
	char inst_buf[2048];
	uint8_t inst_bytes[15];
	xed_decoded_inst_t xed_inst;
	xed_error_enum_t ret_code;
	xed_uint64_t runtime_addr;
	size_t inst_length = 0;
	xed_state_t dstate;
	dstate.mmode = XED_MACHINE_MODE_LONG_64;
	dstate.stack_addr_width = XED_ADDRESS_WIDTH_64b;
	ADDRINT pc_from_ptr = (ADDRINT)pc;
	while(true) {
		xed_decoded_inst_zero_set_mode(&xed_inst, &dstate);
		PIN_SafeCopy(inst_bytes, (VOID*)pc_from_ptr, XED_MAX_INSTRUCTION_BYTES);
		ret_code = xed_decode(&xed_inst, inst_bytes, XED_MAX_INSTRUCTION_BYTES);
		if(ret_code == XED_ERROR_NONE) {
			runtime_addr = static_cast<xed_uint64_t>(pc_from_ptr);
			xed_format_context(
					XED_SYNTAX_INTEL,
					&xed_inst,
					inst_buf, 2048, runtime_addr, NULL, NULL
					);
			inst_length = xed_decoded_inst_get_length(&xed_inst);
			fprintf(stdout,
					" %p [INS]: %s | length: %lu\n",
					(void*)pc_from_ptr, inst_buf, inst_length
					);
			if(strstr(inst_buf, "ret") != NULL) {
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

bool PrecedesRetWithinThreshold(VOID* start_addr, VOID* ret_addr) {
	char inst_buf[2048];
	uint8_t inst_bytes[15];
	xed_decoded_inst_t xed_inst;
	xed_error_enum_t ret_code;
	xed_uint64_t runtime_addr;
	size_t inst_length = 0;
	xed_state_t dstate;
	size_t inst_count = 0;
	dstate.mmode = XED_MACHINE_MODE_LONG_64;
	dstate.stack_addr_width = XED_ADDRESS_WIDTH_64b;
	ADDRINT pc_from_ptr = (ADDRINT)start_addr;
	while(true) {
		xed_decoded_inst_zero_set_mode(&xed_inst, &dstate);
		PIN_SafeCopy(inst_bytes, (VOID*)pc_from_ptr, XED_MAX_INSTRUCTION_BYTES);
		ret_code = xed_decode(&xed_inst, inst_bytes, XED_MAX_INSTRUCTION_BYTES);
		if(ret_code == XED_ERROR_NONE) {
			runtime_addr = static_cast<xed_uint64_t>(pc_from_ptr);
			xed_format_context(
					XED_SYNTAX_INTEL,
					&xed_inst,
					inst_buf, 2048, runtime_addr, NULL, NULL
					);
			inst_length = xed_decoded_inst_get_length(&xed_inst);
			inst_count += 1;
			if(strstr(inst_buf, "ret") != NULL) {
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
	if((VOID*)pc_from_ptr == ret_addr) {
		return inst_length <= GADGET_INS_LENGTH_THRESHOLD;
	} else {
		return false;
	}
}

size_t GetDetectedGadgetCount(const LBR* lbr) {
	if(lbr->GetStackSize() <= 0) return 0;
	size_t chain_count = 0;
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
				&& !IsCallPrecededInstruction(lbr->GetDstAt(i))
		        && PrecedesRetWithinThreshold(prev_branch_target, branch)) {
			chain_count += 1;
			fprintf(stdout,
					"Checking Rb = %p | Rt(n - 1) = %p | Verdict its a gadget!\n",
					branch, prev_branch_target);
			PrintInstUntilRet(prev_branch_target);
		}
		check_count += 1;
		if(i == 0)
			i = lbr->GetStackSize() - 1;
		else
			i -= 1;
	}
	return chain_count; // callers can then check if chain_count >= 1
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
        if(INS_IsRet(ins) || INS_IsFarRet(ins)) {
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
}

VOID CheckForRopBeforeSysCall(THREADID thread_index, CONTEXT* ctxt,
		SYSCALL_STANDARD std, VOID* v) {
	// We do as LBR walk to verify integrity of control flow
	PrintLbrStack(test_lbr);
	size_t rops_detected = GetDetectedGadgetCount(test_lbr);
	fprintf(stdout,
			"# of gadgets detected before syscall(%lu) = %lu\n",
			PIN_GetSyscallNumber(ctxt, std), rops_detected);
}


int main(int argc, char *argv[]) {
    // Initialize pin & symbol manager
    if (PIN_Init(argc, argv)) {
        return -1;
    }
    PIN_InitSymbols();
    xed_tables_init();
    test_lbr = new LBR(16);
    INS_AddInstrumentFunction(InstrumentInstructions, NULL);
    PIN_AddSyscallEntryFunction(CheckForRopBeforeSysCall, NULL);
    PIN_StartProgram();
    return 0;
}
