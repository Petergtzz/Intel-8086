#include "sim86.h"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>

#include "sim86_display.h"
#include "sim86_table.h"
#include "sim86_execute.h"

#include "sim86_display.cpp"

static u8 LoadFileFromMemory(char *FileName, buffer *Buffer)
{
    u8 Result = 0;
    FILE *FilePtr = fopen(FileName, "rb");
    if(FilePtr)
    {
        // Fread returns the number of filled elements in the buffer
        Result = fread(Buffer, 1, ArrayCount(Buffer->Bytes), FilePtr);
        fclose(FilePtr);
    }
    else
    {
        fprintf(stderr, "ERROR: Could not open file");
    }

    return Result;
}

// NOTE (Pedro): Copy contents of buffer into instruction bits stream
static void CopyInstruction(instruction *Instruction, buffer *Buffer, u8 Size)
{
    // Start pointer, points at the first element of the buffer array
    u8 *StartPtr = Buffer->Bytes + Buffer->IndexPtr;

    // Instruction pointer, points at the first element of the array bytes inside instruction, which will hold the bytes per assembly instruction
    memcpy(Instruction->Bits.BytePtr, StartPtr, Size);

    // Increment instruction and buffer pointers
    Instruction->Bits.BytePtr += Size;
    Buffer->IndexPtr += Size;
}

static void ParseRmEncoding(instruction *Instruction,
                            instruction_operand *Operand,
                            buffer *Buffer)
{
    // Memory mode: no displacement follows
    if(Instruction->ModBits == 0b00)
    {
        Operand->Type = Operand_Memory;
        Operand->Memory = {};

        // NOTE (pedro): Special case, 16-bit displacement follows
        if(Instruction->RmBits == 0x6)
        {
            Operand->Memory.Flags.Memory_HasDirectAddress = 0x1;

            // NOTE (pedro): Is direct address always a 16-bit value?
            CopyInstruction(Instruction, Buffer, 2);

            Operand->Memory.DirectAddress =
                (Instruction->Bits.Byte3 << 8) | Instruction->Bits.Byte2;
        }
        else
        {
            Operand->Memory.Register = RegisterLookup[2][Instruction->RmBits];
        }
    }

    // Memory mode: 8-bit displacement follows
    else if(Instruction->ModBits == 0b01)
    {
        Operand->Type = Operand_Memory;
        Operand->Memory = {};
        Operand->Memory.Flags.Memory_HasDisplacement = 0x1;

        // Read 8-bit displacement
        CopyInstruction(Instruction, Buffer, 1);
        Operand->Memory.Displacement = Instruction->Bits.Byte2;

        Operand->Memory.Register = RegisterLookup[2][Instruction->RmBits];

    }

    // Memory mode: 16-bit displacement follows
    else if(Instruction->ModBits == 0b10)
    {
        Operand->Type = Operand_Memory;
        Operand->Memory = {};
        Operand->Memory.Flags.Memory_HasDisplacement = 0x1;

        // Read 16-bit displacement
        CopyInstruction(Instruction, Buffer, 2);
        Operand->Memory.Displacement =
            (Instruction->Bits.Byte3 << 8) | Instruction->Bits.Byte2;

        Operand->Memory.Register = RegisterLookup[2][Instruction->RmBits];
    }

    // Register mode: No displacement follows
    else
    {
        Operand->Type = Operand_Register;
        Operand->Register = RegisterLookup[Instruction->WBit][Instruction->RmBits];
    }
}

static instruction ParseInstruction(buffer *Buffer)
{
    instruction Instruction = {};

    // Read first byte
    CopyInstruction(&Instruction, Buffer, 1);

    // Register/memory to/from register
    if((Instruction.Bits.Byte0 >> 2) == 0b100010)
    {
        Instruction.OpType = mov;

        Instruction.DBit = Instruction.Bits.Byte0 >> 1 & 0b1;
        Instruction.WBit = Instruction.Bits.Byte0 & 0b1;

        // Create source and destination operands
        instruction_operand LeftOperand = {};
        instruction_operand RightOperand = {};

        // Read second byte
        CopyInstruction(&Instruction, Buffer, 1);

        Instruction.ModBits = Instruction.Bits.Byte1 >> 6 & 0b11;
        Instruction.RegBits = Instruction.Bits.Byte1 >> 3 & 0b111;
        Instruction.RmBits = Instruction.Bits.Byte1 & 0b111;

        LeftOperand.Type = Operand_Register;
        LeftOperand.Register = RegisterLookup[Instruction.WBit][Instruction.RegBits];

        // Parse the R/M bits;
        ParseRmEncoding(&Instruction, &RightOperand, Buffer);

        // Left operand is destination and right is source if D bit is on, else the opposite
        if(Instruction.DBit)
        {
            Instruction.Operands[0] = LeftOperand;
            Instruction.Operands[1] = RightOperand;
        }
        else
        {
            Instruction.Operands[0] = RightOperand;
            Instruction.Operands[1] = LeftOperand;
        }
    }

    // Immediate to register/memory
    if((Instruction.Bits.Byte0 >> 1) == 0b1100011)
    {
        Instruction.OpType = mov;

        Instruction.WBit = Instruction.Bits.Byte0 & 0b1;

        // Create source and destination operands
        instruction_operand LeftOperand = {};
        instruction_operand RightOperand = {};

        // Read second byte
        CopyInstruction(&Instruction, Buffer, 1);

        Instruction.ModBits = Instruction.Bits.Byte1 >> 6 & 0b11;
        Instruction.RmBits = Instruction.Bits.Byte1 & 0b111;

        // Parse the R/M bits;
        ParseRmEncoding(&Instruction, &LeftOperand, Buffer);

        RightOperand.Type = Operand_Immediate;

        // TODO (Pedro): FIX WORD BYTE OPERATION!!!
        if(Instruction.WBit)
        {
            // Specify word operation
            RightOperand.Immediate.Flags.Memory_IsWide = 0x2;

            CopyInstruction(&Instruction, Buffer, 2);
            RightOperand.Immediate.Value =
                (Instruction.Bits.Byte3 << 8) | Instruction.Bits.Byte2;
        }
        else
        {
            // Specify byte operation
            RightOperand.Immediate.Flags.Memory_IsWide = 0x1;

            CopyInstruction(&Instruction, Buffer, 1);
            RightOperand.Immediate.Value = Instruction.Bits.Byte2;
        }

        Instruction.Operands[0] = LeftOperand;
        Instruction.Operands[1] = RightOperand;
    }

    // Immediate to register
    if((Instruction.Bits.Byte0 >> 4) == 0b1011)
    {
        Instruction.OpType = mov;

        Instruction.WBit = Instruction.Bits.Byte0 >> 3 & 0b1;
        Instruction.RegBits = Instruction.Bits.Byte0 & 0b111;

        // Create source and destination operands
        instruction_operand LeftOperand = {};
        instruction_operand RightOperand = {};

        LeftOperand.Type = Operand_Register;
        LeftOperand.Register = RegisterLookup[Instruction.WBit][Instruction.RegBits];

        RightOperand.Type = Operand_Immediate;

        if(Instruction.WBit)
        {
            CopyInstruction(&Instruction, Buffer, 2);
            RightOperand.Immediate.Value =
                (Instruction.Bits.Byte2 << 8) | Instruction.Bits.Byte1;
        }
        else
        {
            CopyInstruction(&Instruction, Buffer, 1);
            RightOperand.Immediate.Value = Instruction.Bits.Byte1;
        }

        Instruction.Operands[0] = LeftOperand;
        Instruction.Operands[1] = RightOperand;
    }

    // Memory to accumulator
    if((Instruction.Bits.Byte0 >> 1) == 0b1010000)
    {
        Instruction.OpType = mov;

        Instruction.WBit = Instruction.Bits.Byte0 & 0b1;
        Instruction.RegBits = Instruction.Bits.Byte0 >> 2 & 0b111;

        // Create source and destination operands
        instruction_operand LeftOperand = {};
        instruction_operand RightOperand = {};

        LeftOperand.Type = Operand_Register;
        LeftOperand.Register = RegisterLookup[Instruction.WBit][Instruction.RegBits];

        RightOperand.Type = Operand_Memory;

        if(Instruction.WBit)
        {
            RightOperand.Memory.Flags.Memory_HasDirectAddress = 0x1;

            CopyInstruction(&Instruction, Buffer, 2);
            RightOperand.Memory.DirectAddress =
                (Instruction.Bits.Byte2 << 8) | Instruction.Bits.Byte1;
        }
        else
        {
            RightOperand.Memory.Flags.Memory_HasDirectAddress = 0x1;

            CopyInstruction(&Instruction, Buffer, 1);
            RightOperand.Memory.DirectAddress = Instruction.Bits.Byte1;
        }

        Instruction.Operands[0] = LeftOperand;
        Instruction.Operands[1] = RightOperand;

    }

    // Accumulator to memory
    if((Instruction.Bits.Byte0 >> 1) == 0b1010001)
    {
        Instruction.OpType = mov;

        Instruction.WBit = Instruction.Bits.Byte0 & 0b1;
        Instruction.RegBits = Instruction.Bits.Byte0 >> 2 & 0b111;

        // Create source and destination operands
        instruction_operand LeftOperand = {};
        instruction_operand RightOperand = {};

        LeftOperand.Type = Operand_Register;
        LeftOperand.Register = RegisterLookup[Instruction.WBit][Instruction.RegBits];

        RightOperand.Type = Operand_Memory;

        // Read address
        if(Instruction.WBit)
        {
            CopyInstruction(&Instruction, Buffer, 2);
            RightOperand.Memory.DirectAddress =
                (Instruction.Bits.Byte2 << 8) | Instruction.Bits.Byte1;
            RightOperand.Memory.Flags.Memory_HasDirectAddress = 0x1;
        }
        else
        {
            CopyInstruction(&Instruction, Buffer, 1);
            RightOperand.Memory.DirectAddress = Instruction.Bits.Byte1;
            RightOperand.Memory.Flags.Memory_HasDirectAddress = 0x1;
        }

        Instruction.Operands[0] = RightOperand;
        Instruction.Operands[1] = LeftOperand;
    }

    // ADD / SUB / CMP - Reg/memory with register to either
    if(((Instruction.Bits.Byte0 >> 2) == 0b000000) ||
       ((Instruction.Bits.Byte0 >> 2) == 0b001010) ||
       ((Instruction.Bits.Byte0 >> 2) == 0b001110))
    {
        if((Instruction.Bits.Byte0 >> 3) == 0b000)
        {
            Instruction.OpType = add;
        }
        else if((Instruction.Bits.Byte0 >> 3) == 0b101)
        {
            Instruction.OpType = sub;
        }
        else if((Instruction.Bits.Byte0 >> 3) == 0b111)
        {
            Instruction.OpType = cmp;
        }

        Instruction.DBit = Instruction.Bits.Byte0 >> 1 & 0b1;
        Instruction.WBit = Instruction.Bits.Byte0 & 0b1;

        // Create source and destination operands
        instruction_operand LeftOperand = {};
        instruction_operand RightOperand = {};

        // Read second byte
        CopyInstruction(&Instruction, Buffer, 1);

        Instruction.ModBits = Instruction.Bits.Byte1 >> 6 & 0b11;
        Instruction.RegBits = Instruction.Bits.Byte1 >> 3 & 0b111;
        Instruction.RmBits = Instruction.Bits.Byte1 & 0b111;

        LeftOperand.Type = Operand_Register;
        LeftOperand.Register = RegisterLookup[Instruction.WBit][Instruction.RegBits];

        // Parse the R/M bits;
        ParseRmEncoding(&Instruction, &RightOperand, Buffer);

        if(Instruction.DBit)
        {
            Instruction.Operands[0] = LeftOperand;
            Instruction.Operands[1] = RightOperand;
        }
        else
        {
            Instruction.Operands[0] = RightOperand;
            Instruction.Operands[1] = LeftOperand;
        }
    }

    // ADD / SUB / CMP - Immediate to register/memory
    if((Instruction.Bits.Byte0 >> 2) == 0b100000)
    {
        // Read second byte to determine opcode
        CopyInstruction(&Instruction, Buffer, 1);

        Instruction.SBit = Instruction.Bits.Byte0 >> 1 & 0b1;
        Instruction.WBit = Instruction.Bits.Byte0 & 0b1;

        instruction_operand LeftOperand = {};
        instruction_operand RightOperand = {};

        Instruction.ModBits = Instruction.Bits.Byte1 >> 6 & 0b11;
        Instruction.RegBits = Instruction.Bits.Byte1 >> 3 & 0b111;
        Instruction.RmBits = Instruction.Bits.Byte1 & 0b111;

        if(Instruction.RegBits == 0b000)
        {
            Instruction.OpType = add;
        }
        else if(Instruction.RegBits == 0b101)
        {
            Instruction.OpType = sub;
        }
        else if(Instruction.RegBits == 0b111)
        {
            Instruction.OpType = cmp;
        }

        // One operand is an immediate value
        RightOperand.Type = Operand_Immediate;

        // Parse the R/M bits;
        ParseRmEncoding(&Instruction, &LeftOperand, Buffer);

        if(!Instruction.SBit && Instruction.WBit)
        {
            // Specify word operation
            //RightOperand.Immediate.Flags.Memory_IsWide = 0x2;

            CopyInstruction(&Instruction, Buffer, 2);
            RightOperand.Immediate.Value =
                (Instruction.Bits.Byte3 << 8) | Instruction.Bits.Byte2;
        }
        else
        {
            // Specify byte operation
            //RightOperand.Immediate.Flags.Memory_IsWide = 0x1;

            CopyInstruction(&Instruction, Buffer, 1);
            RightOperand.Immediate.Value = Instruction.Bits.Byte2;
        }

        Instruction.Operands[0] = LeftOperand;
        Instruction.Operands[1] = RightOperand;
    }

    // ADD - Immediate to accumulator
    if((Instruction.Bits.Byte0 >> 1) == 0b0000010)
    {
        Instruction.OpType = add;

        Instruction.WBit = Instruction.Bits.Byte0 & 0b1;
        Instruction.RegBits = Instruction.Bits.Byte0 >> 3 & 0b111;

        // Create source and destination operands
        instruction_operand LeftOperand = {};
        instruction_operand RightOperand = {};

        LeftOperand.Type = Operand_Register;
        LeftOperand.Register = RegisterLookup[Instruction.WBit][Instruction.RegBits];

        RightOperand.Type = Operand_Immediate;

        if(Instruction.WBit)
        {
            //RightOperand.Immediate.Flags.Memory_IsWide = 0x2;

            CopyInstruction(&Instruction, Buffer, 2);
            RightOperand.Immediate.Value =
                (Instruction.Bits.Byte2 << 8) | Instruction.Bits.Byte1;
        }
        else
        {
            //RightOperand.Immediate.Flags.Memory_IsWide = 0x1;

            CopyInstruction(&Instruction, Buffer, 1);
            RightOperand.Immediate.Value = Instruction.Bits.Byte1;
        }

        Instruction.Operands[0] = LeftOperand;
        Instruction.Operands[1] = RightOperand;
    }

    // SUB - Immediate to accumulator
    if((Instruction.Bits.Byte0 >> 1) == 0b0010110)
    {
        Instruction.OpType = sub;

        Instruction.WBit = Instruction.Bits.Byte0 & 0b1;
        Instruction.RegBits = 0b000;

        // Create source and destination operands
        instruction_operand LeftOperand = {};
        instruction_operand RightOperand = {};

        LeftOperand.Type = Operand_Register;
        LeftOperand.Register = RegisterLookup[Instruction.WBit][Instruction.RegBits];

        RightOperand.Type = Operand_Immediate;

        if(Instruction.WBit)
        {
            //RightOperand.Immediate.Flags.Memory_IsWide = 0x2;

            CopyInstruction(&Instruction, Buffer, 2);
            RightOperand.Immediate.Value =
                (Instruction.Bits.Byte2 << 8) | Instruction.Bits.Byte1;
        }
        else
        {
            //RightOperand.Immediate.Flags.Memory_IsWide = 0x1;

            CopyInstruction(&Instruction, Buffer, 1);
            RightOperand.Immediate.Value = Instruction.Bits.Byte1;
        }

        Instruction.Operands[0] = LeftOperand;
        Instruction.Operands[1] = RightOperand;
    }

    // CMP - Immediate to accumulator
    if((Instruction.Bits.Byte0 >> 1) == 0b0011110)
    {
        Instruction.OpType = cmp;

        Instruction.WBit = Instruction.Bits.Byte0 & 0b1;
        Instruction.RegBits = 0b000;

        // Create source and destination operands
        instruction_operand LeftOperand = {};
        instruction_operand RightOperand = {};

        LeftOperand.Type = Operand_Register;
        LeftOperand.Register = RegisterLookup[Instruction.WBit][Instruction.RegBits];

        RightOperand.Type = Operand_Immediate;

        if(Instruction.WBit)
        {
            //RightOperand.Immediate.Flags.Memory_IsWide = 0x2;

            CopyInstruction(&Instruction, Buffer, 2);
            RightOperand.Immediate.Value =
                    (Instruction.Bits.Byte2 << 8) | Instruction.Bits.Byte1;
        }
        else
        {
            //RightOperand.Immediate.Flags.Memory_IsWide = 0x1;

            CopyInstruction(&Instruction, Buffer, 1);
            RightOperand.Immediate.Value = Instruction.Bits.Byte1;
        }

        Instruction.Operands[0] = LeftOperand;
        Instruction.Operands[1] = RightOperand;
    }

    switch(Instruction.Bits.Byte0)
    {
        case 0b01110100:
            Instruction.OpType = je; // JE/JZ
            CopyInstruction(&Instruction, Buffer, 1);
            break;

        case 0b01111100:
            Instruction.OpType = jl; // JL/JNGE
            CopyInstruction(&Instruction, Buffer, 1);
            break;

        case 0b01111110:
            Instruction.OpType = jle; // JLE/JNG
            CopyInstruction(&Instruction, Buffer, 1);
            break;

        case 0b01110010:
            Instruction.OpType = jb; // JB/JNAE
            CopyInstruction(&Instruction, Buffer, 1);
            break;

        case 0b01110110:
            Instruction.OpType = jbe; // JBE/JNA
            CopyInstruction(&Instruction, Buffer, 1);
            break;

        case 0b01111010:
            Instruction.OpType = jp; // JP/JPE
            CopyInstruction(&Instruction, Buffer, 1);
            break;

        case 0b01110000:
            Instruction.OpType = jo; // JO
            CopyInstruction(&Instruction, Buffer, 1);
            break;

        case 0b01111000:
            Instruction.OpType = js; // JS
            CopyInstruction(&Instruction, Buffer, 1);
            break;

        case 0b01110101:
            Instruction.OpType = jne; // JNE/JNZ
            CopyInstruction(&Instruction, Buffer, 1);
            break;

        case 0b01111101:
            Instruction.OpType = jnl; // JNL/JGE
            CopyInstruction(&Instruction, Buffer, 1);
            break;

        case 0b01111111:
            Instruction.OpType = jg; // JNGE/JG
            CopyInstruction(&Instruction, Buffer, 1);
            break;

        case 0b01110011:
            Instruction.OpType = jnb; // JNB/JAE
            CopyInstruction(&Instruction, Buffer, 1);
            break;

        case 0b01110111:
            Instruction.OpType = ja; // JNBE/JA
            CopyInstruction(&Instruction, Buffer, 1);
            break;

        case 0b01111011:
            Instruction.OpType = jnp; // JNP/JPO
            CopyInstruction(&Instruction, Buffer, 1);
            break;

        case 0b01110001:
            Instruction.OpType = jno; // JNO
            CopyInstruction(&Instruction, Buffer, 1);
            break;

        case 0b01111001:
            Instruction.OpType = jns; // JNS
            CopyInstruction(&Instruction, Buffer, 1);
            break;

        case 0b11100010:
            Instruction.OpType = loop; // LOOP
            CopyInstruction(&Instruction, Buffer, 1);
            break;

        case 0b11100001:
            Instruction.OpType = loopz; // LOOPZ
            CopyInstruction(&Instruction, Buffer, 1);
            break;

        case 0b11100000:
            Instruction.OpType = loopnz; // LOOPNZ
            CopyInstruction(&Instruction, Buffer, 1);
            break;

        case 0b11100011:
            Instruction.OpType = jcxz; // JCXZ
            CopyInstruction(&Instruction, Buffer, 1);
            break;
    }

    return Instruction;
}

static void DisAsm8086(u8 BytesRead, buffer *Buffer)
{
    u8 Count = BytesRead;

    // TODO (PEDRO): FIX WHILE LOOP!
    while(Buffer->IndexPtr < Count)
    {
        instruction Instruction = ParseInstruction(Buffer);
        if(Instruction.OpType != op_unknown)
        {
            PrintInstruction(Instruction);
        }
        else
        {
            // TODO (PEDRO): Test for unrecognized input; make sure it works
            //fprintf(stderr, "ERROR: Unrecognized input stream\n");
            break;
        }
    }
}

int main(int ArgCount, char **Args)
{
    bool Execute = false;
    // Allocate memory for each instruction byte
    buffer *Buffer = (buffer *)malloc(sizeof(buffer));

    if(ArgCount > 1)
    {
        char *FileName = Args[1];

        if(strcmp(FileName, "-exec") == 0)
        {
            // TODO (PEDRO): Add execution state
            Execute = true;
        }
        else
        {
            u8 BytesRead = LoadFileFromMemory(FileName, Buffer);

            if(Execute)
            {
                printf("Bits 16\n\n");
                //ExecuteInstruction(BytesRead, Buffer);
            }
            else
            {
                printf("\nDisassembling File: %s\n\n", FileName);
                printf("Bits 16\n\n");
                DisAsm8086(BytesRead, Buffer);
            }
        }

    }
    else
    {
        fprintf(stderr, "USAGE: %s FileName", Args[0]);
    }
    return 0;
}
