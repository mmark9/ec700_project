#include <map>
#include <pin.H>
#include <vector>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include "lbr.h"


std::map<VOID*, std::string> func_name_map;

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
        PrintLbrStack(test_lbr);
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
