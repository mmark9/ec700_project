
#ifndef LBR_H_
#define LBR_H_

#include <pin.H>
#include <stdio.h>
#include <stdlib.h>



class LBR {
public:
    LBR(size_t stack_size);
    ~LBR();

    void AddBranchEntry(VOID* src, VOID* dest);

    size_t GetStackSize() const;
    size_t GetTosPosition() const;

    void PrintLBRStack() const;

    VOID* GetDstAt(size_t i) const;
    VOID* GetSrcAt(size_t i) const;

private:
    size_t _stack_size;
    VOID** _src_msr_stack;
    VOID** _dst_msr_stack;
    size_t _tos_ptr;
};


#endif /* LBR_H_ */
