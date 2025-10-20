#include "utils.h"
#include <string.h>
#include <malloc.h>
#include <stdio.h>

bool8 read_file_to_buffer(const char* filepath, FileBuffer* out_buffer)
{
	FILE* file = fopen(filepath, "rb");
	if(!file)
	{
		perror("Error");
		return false;
	}

	fseek(file, 0, SEEK_END);
	out_buffer->size = ftell(file);
	rewind(file);
	out_buffer->data = malloc(out_buffer->size);

	fread_s(out_buffer->data, out_buffer->size, out_buffer->size, 1, file);
	fclose(file);
    return true;
}

static FILE* out_asm_file = 0;

bool8 asm_file_open(const char* filepath)
{
    out_asm_file = fopen(filepath, "w");
    if(!out_asm_file)
    {
        perror("Error");
        return false;
    }
    fclose(out_asm_file);

    out_asm_file = fopen(filepath, "a");
    if(!out_asm_file)
    {
        perror("Error");
        return false;
    }

    return true;
}

void asm_file_close()
{
    fclose(out_asm_file);
}

void print_out(const char* format, ...)
{
	char line_buf[128];
	va_list args;
	va_start(args, format);
	vsprintf_s(line_buf, sizeof(line_buf), format, args);
	va_end(args);

	printf_s(line_buf);
	fprintf_s(out_asm_file, line_buf);
}

static char op_mnemonic_lookup[OpType_Count][8] =
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

static void _append_char(char* buffer, uint32 buffer_size, char c)
{
    uint32 i = 0;
    while(buffer[i]) i++;
    if (i >= buffer_size - 1)
        return;
        
    buffer[i] = c;
    buffer[i+1] = 0;
}

static bool8 _is_printable(Instruction inst)
{
    return !((inst.op_type == OpType_lock) || (inst.op_type == OpType_rep) || (inst.op_type == OpType_segment));
}

static Register last_register_state[RegisterIndex_Count] = {0};
static uint32 last_inst_cycle_counter = 0;
static uint32 last_ea_cycle_counter = 0;
static uint32 last_transfer_cycle_counter = 0;

void print_instruction(SimulatorContext* context, Instruction inst)
{
    InstructionFlags flags = inst.flags;
    bool8 w = (flags & InstructionFlag_Wide) > 0;

    if (!_is_printable(inst))
        return;

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
            case InstructionOperandType_Accumulator:
            case InstructionOperandType_Register:
            {
                RegisterAccess reg = operand.register_access;
                print_out("%s", reg_name_lookup[reg.reg_index][reg.count == 2 ? 2 : reg.offset]);
                break;
            } 

            case InstructionOperandType_Memory:
            {
                EffectiveAddress address = operand.effective_address;

                if (inst.operands[0].operand_type != InstructionOperandType_Register && inst.operands[0].operand_type != InstructionOperandType_Accumulator)
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
                break;
            }

            case InstructionOperandType_Immediate:
            {
                print_out("%hd", operand.immediate_signed);
                break;
            }

            case InstructionOperandType_RelativeImmediate:
            {
                print_out("$%+hd", operand.immediate_signed);
                break;
            }

            default: 
            {
                break;
            }
        }
    }

    print_out(" ; ");
    separator = "";
    for (uint8 reg_index = RegisterIndex_a; reg_index < RegisterIndex_flags; reg_index++)
    {
        uint16 cur_reg_value = context->registers[reg_index].wide;
        uint16 last_reg_value = last_register_state[reg_index].wide;
        if (cur_reg_value == last_reg_value)
            continue;

        print_out("%s%s(0x%04hx -> 0x%04hx)", separator, reg_name_lookup[reg_index][2], last_reg_value, cur_reg_value);
        separator = ", ";
    }

    uint16 cur_flags_value = context->registers[RegisterIndex_flags].wide;
    uint16 last_flags_value = last_register_state[RegisterIndex_flags].wide;
    if (cur_flags_value != last_flags_value)
    {
        const char flag_letter_lookup[17] = "C P A ZSTIDO    ";
        char cur_flags_s[17] = "";
        for (uint32 shift_i = 0; cur_flags_value; shift_i++, cur_flags_value >>= 1)
        {
            if (cur_flags_value & 1)
                _append_char(cur_flags_s, 17, flag_letter_lookup[shift_i]);
        }
        char last_flags_s[17] = "";
        for (uint32 shift_i = 0; last_flags_value; shift_i++, last_flags_value >>= 1)
        {
            if (last_flags_value & 1)
                _append_char(last_flags_s, 17, flag_letter_lookup[shift_i]);
        }

        print_out("%sflags(%s -> %s)", separator, last_flags_s, cur_flags_s);
    }

    print_out(" ; Total Estimated Cycles: %u (+%u", context->inst_cycle_counter + context->ea_cycle_counter + context->transfer_cycle_counter, context->inst_cycle_counter - last_inst_cycle_counter);
    if (last_ea_cycle_counter != context->ea_cycle_counter) 
        print_out(" + %uea", context->ea_cycle_counter - last_ea_cycle_counter);
    if (last_transfer_cycle_counter != context->transfer_cycle_counter) 
        print_out(" + %ut", context->transfer_cycle_counter - last_transfer_cycle_counter);
    print_out(")");

    print_out("\n");

    memcpy_s(last_register_state, sizeof(last_register_state), context->registers, sizeof(context->registers));
    last_inst_cycle_counter = context->inst_cycle_counter;
    last_ea_cycle_counter = context->ea_cycle_counter;
    last_transfer_cycle_counter = context->transfer_cycle_counter;
}

void print_registers_state(SimulatorContext* context)
{
    const char* header = 
    "-----------------------------------------\n"
    "        | Low   | High  | Wide          | Flags:\n";
    printf(header);

    uint16 flags = context->registers[RegisterIndex_flags].wide;
    char flag_rows[9][32] = {0};

    sprintf_s(flag_rows[0], 32, " Carry: %hhu\n", ((flags & (1 << StatusFlagIndex_Carry)) > 0));
    sprintf_s(flag_rows[1], 32, " Parity: %hhu\n", ((flags & (1 << StatusFlagIndex_Parity)) > 0));
    sprintf_s(flag_rows[2], 32, " AuxCarry: %hhu\n", ((flags & (1 << StatusFlagIndex_AuxCarry)) > 0));
    sprintf_s(flag_rows[3], 32, " Zero: %hhu\n", ((flags & (1 << StatusFlagIndex_Zero)) > 0));
    sprintf_s(flag_rows[4], 32, " Sign: %hhu\n", ((flags & (1 << StatusFlagIndex_Sign)) > 0));
    sprintf_s(flag_rows[5], 32, " Trap: %hhu\n", ((flags & (1 << StatusFlagIndex_Trap)) > 0));
    sprintf_s(flag_rows[6], 32, " InterruptEnable: %hhu\n", ((flags & (1 << StatusFlagIndex_InterruptEnable)) > 0));
    sprintf_s(flag_rows[7], 32, " Direction: %hhu\n", ((flags & (1 << StatusFlagIndex_Direction)) > 0));
    sprintf_s(flag_rows[8], 32, " Overflow: %hhu\n", ((flags & (1 << StatusFlagIndex_Overflow)) > 0));

    for (uint8 reg_index = RegisterIndex_a, line_i = 0; reg_index < RegisterIndex_Count; reg_index++)
    {
        Register reg = context->registers[reg_index];
        printf("-----------------------------------------%s", line_i < 9 ? flag_rows[line_i++] : "\n");
        printf(" %s\t| 0x%02hhx\t| 0x%02hhx\t| 0x%04hx\t|%s",reg_name_lookup[reg_index][2], reg.low, reg.high, reg.wide, line_i < 9 ? flag_rows[line_i++] : "\n");
    }
    printf("-----------------------------------------\n");

}

