#ifndef SIM86_DISPLAY_H
#define SIM86_DISPLAY_H

#include "sim86.h"

static const char *GetMnemonic(operation_types Op);
static const char *GetRegister(register_id Reg);
static void PrintInstruction(instruction Instruction);

#endif
