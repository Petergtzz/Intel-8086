#ifndef SIM86_TABLE_H
#define SIM86_TABLE_H

const char *OpMnemonic[30] =
{
    "mov",
    "add",
    "sub",
    "cmp",
    "jne",
    "je",
    "jl",
    "jle",
    "jb",
    "jbe",
    "jp",
    "jo",
    "js",
    "jnl",
    "jg",
    "jnb",
    "ja",
    "jnp",
    "jno",
    "jns",
    "LOOP",
    "LOOPZ",
    "LOOPNZ",
    "JCXZ",
    "ret",
    "unknown",
};

const char *Registers[30] =
{
    "al",
    "cl",
    "dl",
    "bl",
    "ah",
    "ch",
    "dh",
    "bh",
    "ax",
    "cx",
    "dx",
    "bx",
    "sp",
    "bp",
    "si",
    "di",
    "bx + si",
    "bx + di",
    "bp + si",
    "bp + di",
    "unknown",
};

#endif
