#include "simulator.h"
#include "stdio.h"
#include <malloc.h>
#include <string.h>

static bool8 _execute_mov(SimulatorContext* context, Instruction inst)
{
    bool8 w = (inst.flags & InstructionFlag_Wide) > 0;

    uint8* src = 0;
    uint8 src_size = 0;
    InstructionOperand src_operand = inst.operands[1];
    switch (src_operand.operand_type)
    {
        case InstructionOperandType_Register:
        {
            //src = src_operand.register_access.reg_index



        }
        break;

        default:
        {
            printf("Error: invalid source operand type.");
            return false;
        }
        break;
    }

    return true;
}

void simulator_context_init(SimulatorContext* out_context)
{
    memset(out_context->registers, 0, sizeof(out_context->registers));
    memset(out_context->execute_function_lookup, 0, sizeof(out_context->execute_function_lookup));
    out_context->memory_buffer_size = 1024 * 1024;
    out_context->memory_buffer = malloc(out_context->memory_buffer_size);
    memset(out_context->memory_buffer, 0, out_context->memory_buffer_size);

    out_context->execute_function_lookup[OpType_mov] = _execute_mov;
}

void simulator_context_destroy(SimulatorContext* context)
{
    memset(context->registers, 0, sizeof(context->registers));
    memset(context->execute_function_lookup, 0, sizeof(context->execute_function_lookup));
    free(context->memory_buffer);
    context->memory_buffer = 0;
    context->memory_buffer_size = 0;
}

bool8 simulator_execute_instruction(SimulatorContext* context, Instruction inst)
{
    fp_execute_instruction func = context->execute_function_lookup[inst.op_type];
    if (!func)
        return false;

    return func(context, inst);
}