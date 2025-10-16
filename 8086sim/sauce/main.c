#include "common.h"
#include "decoder.h"
#include "utils.h"

int main(int argc, char** argv)
{
	const char* bin_filepath = 0;
	if (argc < 2)
		bin_filepath = "D:/dev/ComputerEnhance_8086sim/asm/listing_0057_challenge_cycles";
	else
		bin_filepath = argv[1];

	const char* asm_filename = "out.asm";
	if (!asm_file_open(asm_filename))
	{
		printf_s("Failed to open file for writing: %s", asm_filename);
		return 1;
	}

	SimulatorContext sim_context = {0};
	if (!simulator_context_init(&sim_context, bin_filepath))
	{
		printf_s("Failed to initialize simulation context.");
		return 2;
	}
	print_out("; 8086 disassembly for file: \"%s\"\n\nbits 16\n\n", bin_filepath);

	decoder_init();
	uint16 read_offset = sim_context.registers[RegisterIndex_ip].wide;
	while (read_offset < sim_context.instruction_buffer_size)
	{
		Instruction inst = decoder_decode_instruction(&sim_context.memory_buffer[sim_context.instruction_buffer_offset + read_offset], sim_context.instruction_buffer_size - read_offset);
		if (inst.op_type == OpType_none)
		{
			print_out("Error: failed to read next instruction at offset %u.\n", read_offset);
			break;
		}

		sim_context.registers[RegisterIndex_ip].wide += inst.size;
		simulator_execute_instruction(&sim_context, inst);
		print_instruction(&sim_context, inst);

		read_offset = sim_context.registers[RegisterIndex_ip].wide;
	}

	printf("\n");
	print_registers_state(&sim_context);

	simulator_context_destroy(&sim_context);

	printf_s("Done!\n");
	return 0;
}