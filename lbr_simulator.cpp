#include <map>
#include <pin.H>
#include <vector>
#include <string>
#include <stdio.h>
#include <stdlib.h>


std::map<VOID*, std::string> func_name_map;

class LBR {
public:
	LBR(size_t stack_size) {
		_stack_size = stack_size;
		_src_msr_stack = (VOID**)calloc(_stack_size, sizeof(VOID*));
		_dst_msr_stack = (VOID**)calloc(_stack_size, sizeof(VOID*));
		_tos_ptr = -1;
	}

	~LBR() {
		if(_dst_msr_stack) {
			free(_dst_msr_stack);
		}
		if(_src_msr_stack) {
			free(_src_msr_stack);
		}
	}

	void AddBranchEntry(VOID* src, VOID* dest) {
		_tos_ptr = (_tos_ptr + 1) % _stack_size;
		_src_msr_stack[_tos_ptr] = src;
		_dst_msr_stack[_tos_ptr] = dest;
	}

	size_t GetTosPosition() const {
		return _tos_ptr;
	}

	void PrintLBRStack() const {
		int stack_count = 0;
		fprintf(stdout, "\nLBR Stack\n");
		fprintf(stdout, "___________________\n");
		for(int i = _tos_ptr; i >= 0; i--) {
			fprintf(stdout, "[%d] Branch: %p (%s) | Target %p (%s)\n",
					stack_count + 1,
					_src_msr_stack[i],
					func_name_map[_src_msr_stack[i]].c_str(),
					_dst_msr_stack[i],
					func_name_map[_dst_msr_stack[i]].c_str());
			stack_count += 1;
		}
		fprintf(stdout, "___________________\n");
	}

private:
	size_t _stack_size;
	VOID** _src_msr_stack;
	VOID** _dst_msr_stack;
	size_t _tos_ptr;
};

// TODO: create mapping for LBR per thread
LBR* test_lbr = NULL;


void PrintModules(IMG img, VOID* v) {
	size_t image_cnt = 0;
	std::string sym_name_undec;
	for (SYM sym = IMG_RegsymHead(img);
			SYM_Valid(sym); sym = SYM_Next(sym)) {

		sym_name_undec = PIN_UndecorateSymbolName(
				SYM_Name(sym), UNDECORATION_COMPLETE);
		if(sym_name_undec.find('(') == std::string::npos) {
			continue;

		}
		fprintf(stdout, "%lu|%p|%s|%s\n",
				image_cnt + 1,
				(size_t*)SYM_Address(sym),
				SYM_Name(sym).c_str(),
				sym_name_undec.c_str());
		image_cnt += 1;
	}
	exit(0);
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
		test_lbr->PrintLBRStack();
	}
}



void InstrumentInstructions(INS ins, VOID* v) {
	/**
	 * As per the paper, LBR was configured to only
	 * record indirect jmps, rets and calls
	 */
	RTN ins_rtn;
	if(INS_IsIndirectBranchOrCall(ins)) {
		if(INS_IsCall(ins) || INS_IsRet(ins) || INS_IsFarRet(ins)
				|| INS_IsFarJump(ins) || INS_IsBranch(ins)) {
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


int main(int argc, char *argv[]) {
	// Initialize pin & symbol manager
	if (PIN_Init(argc, argv)) {
		return -1;
	}
	PIN_InitSymbols();
	test_lbr = new LBR(16);
	INS_AddInstrumentFunction(InstrumentInstructions, NULL);
	PIN_StartProgram();
	return 0;
}
