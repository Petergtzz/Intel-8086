#ifndef SIM86_H
#define SIM86_H

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;

#define ArrayCount(Array) (sizeof(Array) / sizeof(Array)[0])

typedef enum register_id
{
    al,
    cl,
    dl,
    bl,
    ah,
    ch,
    dh,
    bh,
    ax,
    cx,
    dx,
    bx,
    sp,
    bp,
    si,
    di,
    bx_si,
    bx_di,
    bp_si,
    bp_di,
    unknown,
} register_id;

typedef enum operation_types
{
    mov,
    add,
    sub,
    cmp,
    jne,
    je,
    jl,
    jle,
    jb,
    jbe,
    jp,
    jo,
    js,
    jnl,
    jg,
    jnb,
    ja,
    jnp,
    jno,
    jns,
    loop,
    loopz,
    loopnz,
    jcxz,
    ret,
    op_unknown,
} operation_types;

static register_id RegisterLookup[3][8] =
{
    {
        al,
        cl,
        dl,
        bl,
        ah,
        ch,
        dh,
        bh,
    },
    {
        ax,
        cx,
        dx,
        bx,
        sp,
        bp,
        si,
        di,
    },
    {
        bx_si,
        bx_di,
        bp_si,
        bp_di,
        si,
        di,
        bp,
        bx,
    }
};

typedef enum memory_keyword
{
    byte,
    word,
} keyword;

typedef enum operand_types
{
    Operand_None,
    Operand_Register,
    Operand_Memory,
    Operand_Immediate,
} operand_types;

typedef struct buffer
{
    u8 Bytes[300];
    int IndexPtr;
} buffer;

typedef struct memory_flags
{
    u8 Memory_IsWide;
    u8 Memory_HasDisplacement;
    u8 Memory_HasDirectAddress;
} memory_flags;

typedef struct immediate_flags
{
    u8 Memory_IsWide;
} immediate_flags;

typedef struct operand_memory
{
    register_id Register;
    s16 Displacement;
    u16 DirectAddress;
    memory_flags Flags;
} operand_memory;

typedef struct operand_immediate
{
    u16 Value;
    immediate_flags Flags;
} operand_immediate;

typedef struct instruction_operand
{
    operand_types Type;
    union
    {
        operand_memory Memory;
        register_id Register;
        operand_immediate Immediate;
    };
} instruction_operand;

typedef struct instruction_bits
{
    union
    {
        u8 Bytes[6];
        struct
        {
            u8 Byte0;
            u8 Byte1;
            u8 Byte2;
            u8 Byte3;
            u8 Byte4;
            u8 Byte5;
        };
    };
    u8 *BytePtr = Bytes;
} instruction_bits;

typedef struct instruction
{
    u32 Address;

    operation_types OpType = op_unknown;
    instruction_operand Operands[2] = {};
    instruction_bits Bits;

    u8 DBit;
    u8 WBit;
    u8 ModBits;
    u8 RegBits;
    u8 RmBits;
    u8 SBit;

} instruction;

#endif
