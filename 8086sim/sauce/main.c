#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <malloc.h>

typedef int16_t int16;
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

typedef int32 (*FP_parse_instruction)(Byte* data, uint32 remaining_size);
static FP_parse_instruction primary_instruction_lookup[256] = {0};

// 8-bit, 16-bit register pairs
static char register_name_lookup[8][2][3] = 
{
	[0b000] = {"al","ax"},
	[0b001] = {"cl","cx"},
	[0b010] = {"dl","dx"},
	[0b011] = {"bl","bx"},
	[0b100] = {"ah","sp"},
	[0b101] = {"ch","bp"},
	[0b110] = {"dh","si"},
	[0b111] = {"bh","di"},
};

static void _print_out(FILE* out_file, const char* format, ...);
static void _fill_instruction_lookups();
static int32 _parse_instruction(Byte* data, uint32 remaining_size);

static FILE* out_asm_file = 0;

int main(int argc, char** argv)
{
	const char* bin_filepath = 0;
	if (argc < 2)
		bin_filepath = "D:/dev/ComputerEnhance_8086sim/asm/listing_0039_more_movs";
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

	_fill_instruction_lookups();
	uint32 read_offset = 0;
	while (read_offset < read_buf.size)
	{
		int32 bytes_read = _parse_instruction(&read_buf.data[read_offset], read_buf.size - read_buf.size);
		if (bytes_read < 0)
		{
			_print_out(out_asm_file, "Error: failed to read next instruction at offset %u.\n", read_offset);
			break;
		}

		read_offset += bytes_read;
	}

	fclose(out_asm_file);
	free(read_buf.data);
	printf_s("Done!\n");
	return 0;
}

static void _print_out(FILE* out_file, const char* format, ...)
{
	char line_buf[64];
	va_list args;
	va_start(args, format);
	vsprintf_s(line_buf, sizeof(line_buf), format, args);
	va_end(args);

	printf_s(line_buf);
	fprintf_s(out_file, line_buf);
}

static int32 _parse_instruction(Byte* data, uint32 remaining_size)
{
	FP_parse_instruction parse_inst_func = primary_instruction_lookup[*data];
	if (!parse_inst_func)
		return -1;

	return parse_inst_func(data, remaining_size);
}

// 0b10001d(1)w(1) mod(2)reg(3)r/m(3) {disp-low} {disp-high}
static int32 _inst_mov_mem_reg_transfer(Byte* data, uint32 remaining_size)
{
	static const char* mem_mode_lookup[] =
	{
		[0b000] = "bx + si",
		[0b001] = "bx + di",
		[0b010] = "bp + si",
		[0b011] = "bp + di",
		[0b100] = "si",
		[0b101] = "di",
		[0b110] = "bp",
		[0b111] = "bx",
	};

	Byte byte1 = data[0];
	Byte byte2 = data[1];

	bool8 d = (byte1 >> 1) & 1;
	bool8 w = byte1 & 1;

	uint8 mod_field = byte2 >> 6;
	uint8 reg_field = (byte2 >> 3) & 0b111;
	uint8 rm_field = byte2 & 0b111;

	// Register Mode
	if (mod_field == 0b11)
	{
		uint8 source_reg_id = (reg_field * (!d)) + (rm_field * d);
		uint8 dest_reg_id = (reg_field * d) + (rm_field * (!d));

		_print_out(out_asm_file, "mov %s, %s\n", register_name_lookup[dest_reg_id][w], register_name_lookup[source_reg_id][w]);
		return 2;
	}
	// Direct Address Mem Mode
	else if ((!mod_field) * (rm_field == 0b110))
	{
		if (d)
			_print_out(out_asm_file, "mov %s, [%hu]\n", register_name_lookup[reg_field][w], *((Word*)&data[2]));
		else
			_print_out(out_asm_file, "mov [%hu], %s\n", *((Word*)&data[2]), register_name_lookup[reg_field][w]);

		return 4;
	}
	// Mem Mode w/o Displacement
	else if (!mod_field)
	{
		if (d)
			_print_out(out_asm_file, "mov %s, [%s]\n", register_name_lookup[reg_field][w], mem_mode_lookup[rm_field]);
		else
			_print_out(out_asm_file, "mov [%s], %s\n", mem_mode_lookup[rm_field], register_name_lookup[reg_field][w]);

		return 2;
	}
	// Mem Mode with Displacement
	else
	{
		int16 displacement_value;
		if (mod_field == 0b01)
			displacement_value = (int16)data[2];
		else
			displacement_value = *((int16*)&data[2]);

		if (displacement_value)
		{
			if (d)
				_print_out(out_asm_file, "mov %s, [%s + %hi]\n", register_name_lookup[reg_field][w], mem_mode_lookup[rm_field], displacement_value);
			else
				_print_out(out_asm_file, "mov [%s + %hi], %s\n", mem_mode_lookup[rm_field], displacement_value, register_name_lookup[reg_field][w]);
		}
		else
		{
			if (d)
				_print_out(out_asm_file, "mov %s, [%s]\n", register_name_lookup[reg_field][w], mem_mode_lookup[rm_field]);
			else
				_print_out(out_asm_file, "mov [%s], %s\n", mem_mode_lookup[rm_field], register_name_lookup[reg_field][w]);
		}

		return 2 + mod_field;
	}
}

// 0b1011w(1)reg(3) data-low {data-high}
static int32 _inst_mov_imm_to_reg(Byte* data, uint32 remaining_size)
{
	Byte byte1 = data[0];

	bool8 w = (byte1 >> 3) & 1;
	uint8 reg_field = byte1 & 0b111;
	
	if (w)
	{
		Word imm = *((Word*)&data[1]);
		_print_out(out_asm_file, "mov %s, %hu\n", register_name_lookup[reg_field][w], imm);
		return 3;
	}
	else
	{
		Byte imm = data[1];
		_print_out(out_asm_file, "mov %s, %hhu\n", register_name_lookup[reg_field][w], imm);
		return 2;
	}
}

static void _fill_instruction_lookups()
{
	for (uint32 i = 0b10001000; i <= 0b10001011; i++) primary_instruction_lookup[i] = _inst_mov_mem_reg_transfer;
	for (uint32 i = 0b10110000; i <= 0b10111111; i++) primary_instruction_lookup[i] = _inst_mov_imm_to_reg;
}