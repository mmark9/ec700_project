#include "lbr.h"

LBR::LBR(size_t stack_size) {
	_stack_size = stack_size;
	_src_msr_stack = (VOID**)calloc(_stack_size, sizeof(VOID*));
	_dst_msr_stack = (VOID**)calloc(_stack_size, sizeof(VOID*));
	_tos_ptr = -1;
}

LBR::~LBR() {
	if(_dst_msr_stack) {
		free(_dst_msr_stack);
	}
	if(_src_msr_stack) {
		free(_src_msr_stack);
	}
}

void LBR::AddBranchEntry(VOID* src, VOID* dst) {
	_tos_ptr = (_tos_ptr + 1) % _stack_size;
	_src_msr_stack[_tos_ptr] = src;
	_dst_msr_stack[_tos_ptr] = dst;
}

size_t LBR::GetTosPosition() const{
	return _tos_ptr;
}

VOID* LBR::GetSrcAt(size_t i) const {
	if(i < _stack_size) {
		return _src_msr_stack[i];
	} else {
		return NULL;
	}
}


VOID* LBR::GetDstAt(size_t i) const {
	if(i < _stack_size) {
		return _dst_msr_stack[i];
	} else {
		return NULL;
	}
}
