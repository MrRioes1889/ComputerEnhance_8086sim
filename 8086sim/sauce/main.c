#include <stdio.h>
#include <malloc.h>
#include "common.h"
#include "decoder.h"
#include "utils.h"

int main(int argc, char** argv)
{
	const char* bin_filepath = 0;
	if (argc < 2)
		bin_filepath = "D:/dev/ComputerEnhance_8086sim/asm/listing_0047_challenge_flags";
	else
		bin_filepath = argv[1];

	FileBuffer read_buf = {0};
	if (!read_file_to_buffer(bin_filepath, &read_buf))
	{
		printf_s("Failed to read file %s", bin_filepath);
		return 1;
	}

	const char* asm_filename = "out.asm";
	if (!asm_file_open(asm_filename))
	{
		printf_s("Failed to open file for writing: %s", asm_filename);
		return 1;
	}

	print_out("; 8086 disassembly for file: \"%s\"\n\nbits 16\n\n", bin_filepath);

	SimulatorContext sim_context = {0};
	simulator_context_init(&sim_context);

	decoder_init();
	uint32 read_offset = 0;
	while (read_offset < read_buf.size)
	{
		Instruction inst = decoder_decode_instruction(&read_buf.data[read_offset], read_buf.size - read_buf.size);
		if (inst.op_type == OpType_none)
		{
			print_out("Error: failed to read next instruction at offset %u.\n", read_offset);
			break;
		}

		simulator_execute_instruction(&sim_context, inst);
		print_instruction(&sim_context, inst);
		read_offset += inst.size;
	}

	printf("\n");
	print_registers_state(&sim_context);

	simulator_context_destroy(&sim_context);

	free(read_buf.data);
	printf_s("Done!\n");
	return 0;
}