#ifndef SIM86_DISPLAY_H
#define SIM86_DISPLAY_H

#include "sim86.h"

const char *GetMnemonic(operation_types Op);
const char *GetRegister(register_id Reg);
void PrintInstruction(instruction Instruction);

#endif
