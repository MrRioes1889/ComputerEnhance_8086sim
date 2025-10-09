#include <stdio.h>
#include <malloc.h>
#include "common.h"
#include "decoder2.h"
#include "utils.h"

typedef struct
{
	Byte* data;
	uint32 size;
}
FileBuffer;

static FILE* out_asm_file = 0;

int main(int argc, char** argv)
{
	const char* bin_filepath = 0;
	if (argc < 2)
		bin_filepath = "D:/dev/ComputerEnhance_8086sim/asm/listing_0041_add_sub_cmp_jnz";
	else
		bin_filepath = argv[1];

	FILE* in_binary_file = fopen(bin_filepath, "rb");
	if(!in_binary_file)
	{
		perror("Error");
		printf_s("Failed to read file %s", bin_filepath);
		return 1;
	}

	const char* out_asm_filepath = "out.asm";
	out_asm_file = fopen(out_asm_filepath, "w");
	if(!out_asm_file)
	{
		perror("Error");
		printf_s("Failed to write to file %s", out_asm_filepath);
		return 2;
	}
	fprintf_s(out_asm_file, "; 8086 disassembly for file: \"%s\"\n\nbits 16\n\n", bin_filepath);
	fclose(out_asm_file);

	out_asm_file = fopen(out_asm_filepath, "a");
	if(!out_asm_file)
	{
		perror("Error");
		printf_s("Failed to open file for appending %s", out_asm_filepath);
		return 3;
	}

	FileBuffer read_buf = {0};
	fseek(in_binary_file, 0, SEEK_END);
	read_buf.size = ftell(in_binary_file);
	rewind(in_binary_file);
	read_buf.data = malloc(read_buf.size);

	fread_s(read_buf.data, read_buf.size, read_buf.size, 1, in_binary_file);
	fclose(in_binary_file);

	decoder2_initialize_lookup();
	uint32 read_offset = 0;
	while (read_offset < read_buf.size)
	{
		Instruction inst = decoder2_decode_instruction(&read_buf.data[read_offset], read_buf.size - read_buf.size);
		if (inst.op_type == OpType_none)
		{
			print_out("Error: failed to read next instruction at offset %u.\n", read_offset);
			break;
		}

		print_instruction(inst, out_asm_file);
		read_offset += inst.size;
	}

	fclose(out_asm_file);
	free(read_buf.data);
	printf_s("Done!\n");
	return 0;
}

void print_out(const char* format, ...)
{
	char line_buf[64];
	va_list args;
	va_start(args, format);
	vsprintf_s(line_buf, sizeof(line_buf), format, args);
	va_end(args);

	printf_s(line_buf);
	fprintf_s(out_asm_file, line_buf);
}