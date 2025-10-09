#include "utils.h"

static char op_mnemonic_lookup[OpTypeCount][8] =
{
    [OpType_none] = "",
    #define inst_layout(mnemonic, ...) [OpType_##mnemonic] = #mnemonic,
    #define inst_layout_alt(...)
    #include "instruction_layouts.inl"
};

static char reg_name_lookup[][3][6] =
{
    [RegisterIndex_None] = {"", "", ""},
    [RegisterIndex_a] = {"al", "ah", "ax"},
    [RegisterIndex_b] = {"bl", "bh", "bx"},
    [RegisterIndex_c] = {"cl", "ch", "cx"},
    [RegisterIndex_d] = {"dl", "dh", "dx"},
    [RegisterIndex_sp] = {"sp", "sp", "sp"},
    [RegisterIndex_bp] = {"bp", "bp", "bp"},
    [RegisterIndex_si] = {"si", "si", "si"},
    [RegisterIndex_di] = {"di", "di", "di"},
    [RegisterIndex_es] = {"es", "es", "es"},
    [RegisterIndex_cs] = {"cs", "cs", "cs"},
    [RegisterIndex_ss] = {"ss", "ss", "ss"},
    [RegisterIndex_ds] = {"ds", "ds", "ds"},
    [RegisterIndex_ip] = {"ip", "ip", "ip"},
    [RegisterIndex_flags] = {"flags", "flags", "flags"},
};
static_assert(array_count(reg_name_lookup) == RegisterIndex_Count, "reg name lookup should cover all registers");

static char address_base_lookup[][6] =
{
    [EffectiveAddressBase_direct] = "",
    [EffectiveAddressBase_bx_si] = "bx+si",
    [EffectiveAddressBase_bx_di] = "bx+di",
    [EffectiveAddressBase_bp_si] = "bp+si",
    [EffectiveAddressBase_bp_di] = "bp+di",
    [EffectiveAddressBase_si] = "si",
    [EffectiveAddressBase_di] = "di",
    [EffectiveAddressBase_bp] = "bp",
    [EffectiveAddressBase_bx] = "bx",
};
static_assert(array_count(address_base_lookup) == EffectiveAddressBase_Count, "address base string lookup should cover all address bases");

static bool8 _is_printable(Instruction inst)
{
    return ((inst.op_type == OpType_lock) || (inst.op_type == OpType_rep) || (inst.op_type == OpType_segment));
}

void print_instruction(Instruction inst, FILE* asm_file)
{
    InstructionFlags flags = inst.flags;
    bool8 w = (flags & InstructionFlag_Wide) > 0;

    if (flags & InstructionFlag_Lock)
    {
        if (inst.op_type == OpType_xchg)
        {
            InstructionOperand tmp = inst.operands[0];
            inst.operands[0] = inst.operands[1];
            inst.operands[1] = tmp;
        }

        print_out("lock ");
    }

    const char* mnemonic_suffix = "";
    if (flags & InstructionFlag_Rep)
    {
        print_out("rep ");
        mnemonic_suffix = w ? "w" : "b";
    }

    print_out("%s%s ", op_mnemonic_lookup[inst.op_type], mnemonic_suffix);

    const char* separator = "";
    for (uint32 operand_i = 0; operand_i < array_count(inst.operands); operand_i++)
    {
        InstructionOperand operand = inst.operands[operand_i];
        if (operand.operand_type == InstructionOperandType_None)
            continue;

        print_out("%s", separator);
        separator = ", "; 

        switch(operand.operand_type)
        {
            case InstructionOperandType_Register:
            {
                RegisterAccess reg = operand.register_access;
                print_out("%s", reg_name_lookup[reg.reg_index][reg.count == 2 ? 2 : reg.offset]);
            } 
            break;

            case InstructionOperandType_Memory:
            {
                EffectiveAddress address = operand.effective_address;

                if (inst.operands[0].operand_type != InstructionOperandType_Register)
                    print_out("%s ", w ? "word" : "byte");

                if (flags & InstructionFlag_Segment)
                    print_out("%s:", reg_name_lookup[address.segment][2]);

                if (address.base == EffectiveAddressBase_direct)
                {
                    print_out("[%hd]", address.displacement);
                }
                else
                {
                    print_out("[%s", address_base_lookup[address.base]);
                    if (address.displacement)
                        print_out("%+hd", address.displacement);
                    print_out("]");
                }
            }
            break;

            case InstructionOperandType_Immediate:
            {
                print_out("%hd", operand.immediate_signed);
            }
            break;

            case InstructionOperandType_RelativeImmediate:
            {
                print_out("$%+hd", operand.immediate_signed);
            }
            break;

            default: 
            {}
            break;
        }
    }

    print_out("\n");
}