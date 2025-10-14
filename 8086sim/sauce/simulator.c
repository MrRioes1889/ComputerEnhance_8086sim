#include "simulator.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>

typedef bool8(*fp_execute_instruction)(SimulatorContext* context, Instruction inst);
static fp_execute_instruction execute_function_lookup[OpType_Count] = {0};

static void _update_status_flag(SimulatorContext* context, StatusFlagIndex flag_index, bool8 new_flag_value)
{
    if (new_flag_value)
        context->registers[RegisterIndex_flags].wide |= (1 << flag_index);
    else
        context->registers[RegisterIndex_flags].wide &= ~(1 << flag_index);
}

static bool8 _execute_mov(SimulatorContext* context, Instruction inst)
{
    bool8 w = (inst.flags & InstructionFlag_Wide) > 0;

    uint16 value = 0;
    InstructionOperand src_operand = inst.operands[1];
    switch (src_operand.operand_type)
    {
        case InstructionOperandType_Register:
        {
            RegisterAccess acc = src_operand.register_access;
            Register reg = context->registers[acc.reg_index];
            value = acc.count == 2 ? reg.wide : reg.sub_reg[acc.offset];
            break;
        }

        case InstructionOperandType_Immediate:
        {
            value = src_operand.immediate_unsigned;
            break;
        }

        default:
        {
            printf("Error: invalid source operand type.\n");
            return false;
        }
    }

    InstructionOperand dst_operand = inst.operands[0];
    switch (dst_operand.operand_type)
    {
        case InstructionOperandType_Register:
        {
            RegisterAccess acc = dst_operand.register_access;
            Register* reg = &context->registers[acc.reg_index];
            if (acc.count == 2)
                reg->wide = value;
            else
                reg->sub_reg[acc.offset] = (uint8)value;

            break;
        }

        default:
        {
            printf("Error: Invalid dest operand type.\n");
            return false;
        }
    }

    return true;
}

static uint16 _extract_src_value(SimulatorContext* context, InstructionOperand src_operand)
{
    uint16 src_value = 0;
    switch (src_operand.operand_type)
    {
        case InstructionOperandType_Register:
        {
            RegisterAccess acc = src_operand.register_access;
            Register reg = context->registers[acc.reg_index];
            src_value = acc.count == 2 ? reg.wide : reg.sub_reg[acc.offset];
            break;
        }

        case InstructionOperandType_Immediate:
        {
            src_value = src_operand.immediate_unsigned;
            break;
        }

        default:
        {
            printf("Error: invalid source operand type.\n");
            return false;
        }
    }

    return src_value;
}

static bool8 _execute_add(SimulatorContext* context, Instruction inst)
{
    uint16 src_value = _extract_src_value(context, inst.operands[1]);

    uint8 dest_size = 0;
    uint16 orig_dest_value = 0;
    uint16 new_dest_value = 0;

    InstructionOperand dst_operand = inst.operands[0];
    switch (dst_operand.operand_type)
    {
        case InstructionOperandType_Register:
        {
            RegisterAccess acc = dst_operand.register_access;
            Register* reg = &context->registers[acc.reg_index];
            dest_size = acc.count;
            if (acc.count == 2)
            {
                orig_dest_value = reg->wide;
                reg->wide += src_value;
                new_dest_value = reg->wide;
            }
            else
            {
                orig_dest_value = reg->sub_reg[acc.offset];
                reg->sub_reg[acc.offset] += (uint8)src_value;
                new_dest_value = reg->sub_reg[acc.offset];
            }

            break;
        }

        default:
        {
            printf("Error: Invalid dest operand type.\n");
            return false;
        }
    }

    _update_status_flag(context, StatusFlagIndex_Carry, (new_dest_value < orig_dest_value));
    bool8 parity = true;
    for (uint16 value = new_dest_value, bit_i = 0; value && bit_i < 8; value = value >> 1, bit_i++)
        parity = (value & 1) ? !parity : parity;
    _update_status_flag(context, StatusFlagIndex_Parity, parity);
    _update_status_flag(context, StatusFlagIndex_AuxCarry, false);
    _update_status_flag(context, StatusFlagIndex_Zero, (new_dest_value == 0));
    uint8 highest_bit_shift = (8 * dest_size) - 1;
    _update_status_flag(context, StatusFlagIndex_Sign, ((new_dest_value >> highest_bit_shift) > 0));
    _update_status_flag(context, StatusFlagIndex_Overflow, (new_dest_value >> highest_bit_shift) != (orig_dest_value >> highest_bit_shift));

    return true;
}

static bool8 _execute_sub(SimulatorContext* context, Instruction inst)
{
    uint16 src_value = _extract_src_value(context, inst.operands[1]);

    uint8 dest_size = 0;
    uint16 orig_dest_value = 0;
    uint16 new_dest_value = 0;

    InstructionOperand dst_operand = inst.operands[0];
    switch (dst_operand.operand_type)
    {
        case InstructionOperandType_Register:
        {
            RegisterAccess acc = dst_operand.register_access;
            Register* reg = &context->registers[acc.reg_index];
            dest_size = acc.count;
            if (acc.count == 2)
            {
                orig_dest_value = reg->wide;
                reg->wide -= src_value;
                new_dest_value = reg->wide;
            }
            else
            {
                orig_dest_value = reg->sub_reg[acc.offset];
                reg->sub_reg[acc.offset] -= (uint8)src_value;
                new_dest_value = reg->sub_reg[acc.offset];
            }

            break;
        }

        default:
        {
            printf("Error: Invalid dest operand type.\n");
            return false;
        }
    }

    _update_status_flag(context, StatusFlagIndex_Carry, (new_dest_value > orig_dest_value));
    bool8 parity = true;
    for (uint16 value = new_dest_value, bit_i = 0; value && bit_i < 8; value = value >> 1, bit_i++)
        parity = (value & 1) ? !parity : parity;
    _update_status_flag(context, StatusFlagIndex_Parity, parity);
    _update_status_flag(context, StatusFlagIndex_AuxCarry, false);
    _update_status_flag(context, StatusFlagIndex_Zero, (new_dest_value == 0));
    uint8 highest_bit_shift = (8 * dest_size) - 1;
    _update_status_flag(context, StatusFlagIndex_Sign, ((new_dest_value >> highest_bit_shift) > 0));
    _update_status_flag(context, StatusFlagIndex_Overflow, (new_dest_value >> highest_bit_shift) != (orig_dest_value >> highest_bit_shift));

    return true;
}

static bool8 _execute_cmp(SimulatorContext* context, Instruction inst)
{
    uint16 src_value = _extract_src_value(context, inst.operands[1]);

    uint8 dest_size = 0;
    uint16 dest_value = 0;
    uint16 cmp_value = 0;

    InstructionOperand dst_operand = inst.operands[0];
    switch (dst_operand.operand_type)
    {
        case InstructionOperandType_Register:
        {
            RegisterAccess acc = dst_operand.register_access;
            Register* reg = &context->registers[acc.reg_index];
            dest_size = acc.count;
            if (acc.count == 2)
            {
                dest_value = reg->wide;
                cmp_value = dest_value - src_value;
            }
            else
            {
                dest_value = reg->sub_reg[acc.offset];
                cmp_value = dest_value - (uint8)src_value;
            }

            break;
        }

        default:
        {
            printf("Error: Invalid dest operand type.\n");
            return false;
        }
    }

    _update_status_flag(context, StatusFlagIndex_Carry, (dest_value > cmp_value));
    bool8 parity = true;
    for (uint16 value = cmp_value, bit_i = 0; value && bit_i < 8; value = value >> 1, bit_i++)
        parity = (value & 1) ? !parity : parity;
    _update_status_flag(context, StatusFlagIndex_Parity, parity);
    _update_status_flag(context, StatusFlagIndex_AuxCarry, false);
    _update_status_flag(context, StatusFlagIndex_Zero, (cmp_value == 0));
    uint8 highest_bit_shift = (8 * dest_size) - 1;
    _update_status_flag(context, StatusFlagIndex_Sign, ((cmp_value >> highest_bit_shift) > 0));
    _update_status_flag(context, StatusFlagIndex_Overflow, (cmp_value >> highest_bit_shift) != (dest_value >> highest_bit_shift));

    return true;
}

static bool8 _execute_jz(SimulatorContext* context, Instruction inst)
{
    uint16 flags = context->registers[RegisterIndex_flags].wide;
    if (flags & (1 << StatusFlagIndex_Zero)) context->registers[RegisterIndex_ip].wide += inst.operands[0].immediate_signed - inst.size;
    return true;
}

static bool8 _execute_jnz(SimulatorContext* context, Instruction inst)
{
    uint16 flags = context->registers[RegisterIndex_flags].wide;
    if (!(flags & (1 << StatusFlagIndex_Zero))) context->registers[RegisterIndex_ip].wide += inst.operands[0].immediate_signed - inst.size;
    return true;
}

static bool8 _execute_jp(SimulatorContext* context, Instruction inst)
{
    uint16 flags = context->registers[RegisterIndex_flags].wide;
    if (flags & (1 << StatusFlagIndex_Parity)) context->registers[RegisterIndex_ip].wide += inst.operands[0].immediate_signed - inst.size;
    return true;
}

static bool8 _execute_jb(SimulatorContext* context, Instruction inst)
{
    uint16 flags = context->registers[RegisterIndex_flags].wide;
    if (flags & (1 << StatusFlagIndex_Carry)) context->registers[RegisterIndex_ip].wide += inst.operands[0].immediate_signed - inst.size;
    return true;
}

static bool8 _execute_loopnz(SimulatorContext* context, Instruction inst)
{
    context->registers[RegisterIndex_c].wide--;
    uint16 flags = context->registers[RegisterIndex_flags].wide;
    if (context->registers[RegisterIndex_c].wide > 0 && !(flags & (1 << StatusFlagIndex_Zero))) context->registers[RegisterIndex_ip].wide += inst.operands[0].immediate_signed - inst.size;
    return true;
}

bool8 simulator_context_init(SimulatorContext* out_context, const char* bin_filepath)
{
    memset(out_context->registers, 0, sizeof(out_context->registers));
    out_context->memory_buffer_size = 1024 * 1024;
    out_context->memory_buffer = malloc(out_context->memory_buffer_size);
    memset(out_context->memory_buffer, 0, out_context->memory_buffer_size);

    execute_function_lookup[OpType_mov] = _execute_mov;
    execute_function_lookup[OpType_add] = _execute_add;
    execute_function_lookup[OpType_sub] = _execute_sub;
    execute_function_lookup[OpType_cmp] = _execute_cmp;
    execute_function_lookup[OpType_jz] = _execute_jz;
    execute_function_lookup[OpType_jnz] = _execute_jnz;
    execute_function_lookup[OpType_jp] = _execute_jp;
    execute_function_lookup[OpType_jb] = _execute_jb;
    execute_function_lookup[OpType_loopnz] = _execute_loopnz;

	FILE* file = fopen(bin_filepath, "rb");
	if(!file)
	{
		perror("Error");
		return false;
	}

	fseek(file, 0, SEEK_END);
	out_context->instruction_buffer_size = ftell(file);
	rewind(file);
    assert(out_context->instruction_buffer_size < 0xFFFF);

    out_context->instruction_buffer_offset = 0;
	fread_s(&out_context->memory_buffer[out_context->instruction_buffer_offset], out_context->instruction_buffer_size, out_context->instruction_buffer_size, 1, file);
	fclose(file);

    return true;
}

void simulator_context_destroy(SimulatorContext* context)
{
    memset(context->registers, 0, sizeof(context->registers));
    free(context->memory_buffer);
    context->memory_buffer = 0;
    context->memory_buffer_size = 0;
}

bool8 simulator_execute_instruction(SimulatorContext* context, Instruction inst)
{
    fp_execute_instruction func = execute_function_lookup[inst.op_type];
    if (!func)
        return false;

    return func(context, inst);
}