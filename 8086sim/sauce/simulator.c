#include "simulator.h"
#include "stdio.h"
#include <malloc.h>
#include <string.h>

typedef bool8(*fp_execute_instruction)(SimulatorContext* context, Instruction inst);
static fp_execute_instruction execute_function_lookup[OpType_Count] = {0};

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
            printf("Error: invalid source operand type.");
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
            printf("Error: Invalid dest operand type.");
            return false;
        }
    }

    return true;
}

void simulator_context_init(SimulatorContext* out_context)
{
    memset(out_context->registers, 0, sizeof(out_context->registers));
    out_context->memory_buffer_size = 1024 * 1024;
    out_context->memory_buffer = malloc(out_context->memory_buffer_size);
    memset(out_context->memory_buffer, 0, out_context->memory_buffer_size);

    execute_function_lookup[OpType_mov] = _execute_mov;
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