#pragma once
#include "common.h"

enum
{
    OpType_none,
    #define inst_layout(mnemonic, ...) OpType_##mnemonic,
    #define inst_layout_alt(...)
    #include "instruction_layouts.inl"
    OpType_Count
};
typedef uint8 OpTypeValue;

enum
{
    InstructionFlag_Lock = (1 << 0),
    InstructionFlag_Rep = (1 << 1),
    InstructionFlag_Segment = (1 << 2),
    InstructionFlag_Wide = (1 << 3),
};
typedef uint8 InstructionFlags;

enum
{
    RegisterIndex_None,

    RegisterIndex_a,
    RegisterIndex_b,
    RegisterIndex_c,
    RegisterIndex_d,
    RegisterIndex_sp,
    RegisterIndex_bp,
    RegisterIndex_si,
    RegisterIndex_di,
    RegisterIndex_es,
    RegisterIndex_cs,
    RegisterIndex_ss,
    RegisterIndex_ds,
    RegisterIndex_ip,
    RegisterIndex_flags,

    RegisterIndex_Count,
};
typedef uint8 RegisterIndex;

enum
{
    EffectiveAddressBase_direct,

    EffectiveAddressBase_bx_si,
    EffectiveAddressBase_bx_di,
    EffectiveAddressBase_bp_si,
    EffectiveAddressBase_bp_di,
    EffectiveAddressBase_si,
    EffectiveAddressBase_di,
    EffectiveAddressBase_bp,
    EffectiveAddressBase_bx,

    EffectiveAddressBase_Count,
};
typedef uint8 EffectiveAddressBase;

enum
{
    InstructionOperandType_None,
    InstructionOperandType_Register,
    InstructionOperandType_Memory,
    InstructionOperandType_Immediate,
    InstructionOperandType_RelativeImmediate
};
typedef uint8 InstructionOperandType;

typedef struct 
{
    RegisterIndex segment;
    EffectiveAddressBase base;
    int16 displacement;
}
EffectiveAddress;

typedef struct
{
    RegisterIndex reg_index;
    uint8 offset;
    uint8 count;
}
RegisterAccess;

typedef struct
{
    InstructionOperandType operand_type;
    union
    {
        EffectiveAddress effective_address;
        RegisterAccess register_access;
        uint16 immediate_unsigned;
        int16 immediate_signed;
    };
}
InstructionOperand;

typedef struct
{
    uint16 address;
    uint8 size;
    OpTypeValue op_type;
    uint8 flags;

    InstructionOperand operands[2];
}
Instruction;

typedef struct
{
    union
    {
        uint16 wide;
        struct
        {
            uint8 low;
            uint8 high;
        };
        uint8 sub_reg[2];
    };
}
Register;

typedef struct
{
    Register registers[RegisterIndex_Count];
    uint32 memory_buffer_size;
    void* memory_buffer;
}
SimulatorContext;

enum
{
    StatusFlagIndex_Carry = 0,
    StatusFlagIndex_Parity = 2,
    StatusFlagIndex_AuxCarry = 4,
    StatusFlagIndex_Zero = 6,
    StatusFlagIndex_Sign = 7,
    StatusFlagIndex_Trap = 8,
    StatusFlagIndex_InterruptEnable = 9,
    StatusFlagIndex_Direction = 10,
    StatusFlagIndex_Overflow = 11,
};
typedef uint16 StatusFlagIndex;

void simulator_context_init(SimulatorContext* out_context);
void simulator_context_destroy(SimulatorContext* context);

bool8 simulator_execute_instruction(SimulatorContext* context, Instruction inst);
