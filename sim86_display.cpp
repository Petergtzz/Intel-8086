#include "sim86_display.h"
#include "sim86_table.h"

static const char *GetMnemonic(operation_types Op)
{
    const char *Result = OpMnemonic[Op];
    return Result;
}

static const char *GetRegister(register_id Reg)
{
    const char *Result = Registers[Reg];
    return Result;
}

static void PrintInstruction(instruction Instruction)
{
    const char *Mnemonic = GetMnemonic(Instruction.OpType);
    printf("%s ", Mnemonic);
    const char *Separator = "";

    for(int Index = 0; Index < ArrayCount(Instruction.Operands); Index++)
    {
        instruction_operand Operand = Instruction.Operands[Index];

        printf("%s", Separator);
        Separator = ", ";

        switch(Operand.Type)
        {
            case Operand_None:
            {
            } break;

            case Operand_Memory:
            {
                printf("[");
                const char *Register = GetRegister(Operand.Register);

                if(!Operand.Memory.Flags.Memory_HasDirectAddress)
                {
                    printf("%s", Register);
                }

                if(Operand.Memory.Flags.Memory_HasDisplacement)
                {
                    if(Operand.Memory.Displacement >= 0)
                    {
                        printf(" + %i", Operand.Memory.Displacement);
                    }
                    else
                    {
                        printf(" - %i", Operand.Memory.Displacement * -1);
                    }
                }

                if(Operand.Memory.Flags.Memory_HasDirectAddress)
                {
                    printf("%d", Operand.Memory.DirectAddress);
                }

                printf("]");
            } break;

            case Operand_Immediate:
            {
                if(Operand.Immediate.Flags.Memory_IsWide)
                {
                    printf("%s ", (Operand.Immediate.Flags.Memory_IsWide == 0x2) ? "word" : "byte");
                }

                printf("%d", Operand.Immediate.Value);

            } break;

            case Operand_Register:
            {
                // TODO (PEDRO): Fix the separator and the next line printing
                const char *Register = GetRegister(Operand.Register);
                printf("%s", Register);
            } break;

            default:
            {
            } break;
        }
    }
    printf("\n");
}
