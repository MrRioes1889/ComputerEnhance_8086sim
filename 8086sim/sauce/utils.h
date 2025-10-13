#pragma once
#include "common.h"
#include <stdio.h>
#include "simulator.h"

typedef struct
{
	Byte* data;
	uint32 size;
}
FileBuffer;

bool8 read_file_to_buffer(const char* filepath, FileBuffer* out_buffer);
void print_out(const char* format, ...);

bool8 asm_file_open(const char* filepath);
void asm_file_close();

void print_instruction(SimulatorContext* context, Instruction inst);
void print_registers_state(SimulatorContext* context);