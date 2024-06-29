#ifndef SIM86_EXECUTE_H
#define SIM86_EXECUTE_H

#include "sim86.h"
#include "sim86_table.h"

typedef union {

// The double ## concatenates the passed letter to form il, ih, ix to specify the lower, higher or full register bits
#define REG16(i) union {struct{u8 i##l; u8 i##h;}; u16 i##x;}

    struct
    {
        REG16(a);
        REG16(b);
        REG16(c);
        REG16(d);
        u16 sp;
        u16 bp;
        u16 si;
        u16 di;
    };

#undef REG16

} regs;

void ExecuteInstruction(u8 BytesRead, buffer *Buffer);

#endif
