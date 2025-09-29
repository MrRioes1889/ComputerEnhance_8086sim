#include <stdio.h>
#include <stdint.h>
#include <malloc.h>

typedef int32_t int32;
typedef uint8_t bool8;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef uint8_t Byte;
typedef uint16_t Word;

typedef struct
{
	Byte* data;
	uint32 size;
}
FileBuffer;

typedef int32 (*FP_parse_instruction)(FileBuffer buffer, uint32 read_offset);
static FP_parse_instruction primary_instruction_lookup[256] = {0};

#if 0
static const char* register_name_lookup[16] = 
{
	// 8-bit registers
	[0b0000] = "al",
	[0b0001] = "cl",
	[0b0010] = "dl",
	[0b0011] = "bl",
	[0b0100] = "ah",
	[0b0101] = "ch",
	[0b0110] = "dh",
	[0b0111] = "bh",

	// 16-bit registers
	[0b1000] = "ax",
	[0b1001] = "cx",
	[0b1010] = "dx",
	[0b1011] = "bx",
	[0b1100] = "sp",
	[0b1101] = "bp",
	[0b1110] = "si",
	[0b1111] = "di",
};
#else
static char register_name_lookup[][2][3] = 
{
	// 8-bit and 16-bit registers
	[0b000] = {"al","ax"},
	[0b001] = {"cl","cx"},
	[0b010] = {"dl","dx"},
	[0b011] = {"bl","bx"},
	[0b100] = {"ah","sp"},
	[0b101] = {"ch","bp"},
	[0b110] = {"dh","si"},
	[0b111] = {"bh","di"},
};
#endif

static void _fill_instruction_lookups();

static int32 _parse_instruction(FileBuffer buffer, uint32 read_offset);

static FILE* out_asm_file = 0;

int main(int argc, char** argv)
{
	const char* bin_filepath = 0;
	if (argc < 2)
		bin_filepath = "D:/dev/ComputerEnhance_8086sim/asm/listing_0038_many_register_mov";
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
		printf_s("Failed to oprn file for appending %s", out_asm_filepath);
		return 2;
	}

	FileBuffer read_buf = {0};
	fseek(in_binary_file, 0, SEEK_END);
	read_buf.size = ftell(in_binary_file);
	rewind(in_binary_file);
	read_buf.data = malloc(read_buf.size);

	fread_s(read_buf.data, read_buf.size, read_buf.size, 1, in_binary_file);
	fclose(in_binary_file);

	printf("File Bytes:\n");

	_fill_instruction_lookups();
	uint32 read_offset = 0;
	while (read_offset < read_buf.size)
	{
		int32 bytes_read = _parse_instruction(read_buf, read_offset);
		if (bytes_read < 0)
		{
			printf_s("Error: failed to read next instruction at offset %u.", read_offset);
			fprintf_s(out_asm_file, "Error: failed to read next instruction at offset %u.", read_offset);
			break;
		}

		read_offset += bytes_read;
	}

	fclose(out_asm_file);
	free(read_buf.data);
	printf_s("Done!\n");
	return 0;
}

static int32 _parse_instruction(FileBuffer buffer, uint32 read_offset)
{
	FP_parse_instruction parse_inst_func = primary_instruction_lookup[buffer.data[read_offset]];
	if (!parse_inst_func)
		return -1;

	return parse_inst_func(buffer, read_offset);
}

// 0b10001dw mod(2)reg(3)r/m(3)
static int32 _inst_mov_mem_reg_transfer(FileBuffer buffer, uint32 in_read_offset)
{
	uint32 read_offset = in_read_offset;
	Byte byte1 = buffer.data[read_offset++];
	Byte byte2 = buffer.data[read_offset++];
	uint8 displacement_byte_count = 0;
	Byte displacements_bytes[2] = {0};

	bool8 d = (byte1 >> 1) & 1;
	bool8 w = byte1 & 1;

	uint8 mode_field = byte2 >> 6;
	uint8 reg_field = (byte2 >> 3) & 0b111;
	uint8 rm_field = byte2 & 0b111;
	// Register Mode
	if (mode_field == 0b11)
	{
		uint8 source_reg_id = (reg_field * (!d)) + (rm_field * d);
		uint8 dest_reg_id = (reg_field * d) + (rm_field * (!d));

		printf_s("mov %s, %s\n", register_name_lookup[dest_reg_id][w], register_name_lookup[source_reg_id][w]);
		fprintf_s(out_asm_file, "mov %s, %s\n", register_name_lookup[dest_reg_id][w], register_name_lookup[source_reg_id][w]);
	}
	else
	{
		displacement_byte_count += mode_field;
		printf_s("Error: non register mov not supported yet!");
		return -1;
	}

	#if 0
	printf("%02x %02x", byte1, byte2);
	for (uint8 i = 0; i < displacement_byte_count; i++)
		printf(" %02x", displacements_bytes[i]);
	printf("\n");
	#endif
	
	return 2 + displacement_byte_count;
}

static void _fill_instruction_lookups()
{
	for (uint32 i = 0b10001000; i <= 0b10001011; i++)
		primary_instruction_lookup[i] = _inst_mov_mem_reg_transfer;
}