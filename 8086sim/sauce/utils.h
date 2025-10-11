#pragma once
#include "common.h"
#include <stdio.h>
#include "simulator.h"

void print_instruction(SimulatorContext* context, Instruction inst, FILE* asm_file);
void print_registers_state(SimulatorContext* context);