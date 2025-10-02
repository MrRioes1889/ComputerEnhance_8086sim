#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <malloc.h>
#include <math.h>

typedef int8_t int8;
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

static const char* mem_mode_rm_field_reg_lookup[] =
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


static void _print_out(const char* format, ...);
static void _fill_instruction_lookups();
static int32 _parse_instruction(Byte* data, uint32 remaining_size);

static FILE* out_asm_file = 0;

int main(int argc, char** argv)
{
	const char* bin_filepath = 0;
	if (argc < 2)
		bin_filepath = "D:/dev/ComputerEnhance_8086sim/asm/listing_0040_challenge_movs";
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

	_fill_instruction_lookups();
	uint32 read_offset = 0;
	while (read_offset < read_buf.size)
	{
		int32 bytes_read = _parse_instruction(&read_buf.data[read_offset], read_buf.size - read_buf.size);
		if (bytes_read < 0)
		{
			_print_out("Error: failed to read next instruction at offset %u.\n", read_offset);
			break;
		}

		read_offset += bytes_read;
	}

	fclose(out_asm_file);
	free(read_buf.data);
	printf_s("Done!\n");
	return 0;
}

static void _print_out(const char* format, ...)
{
	char line_buf[64];
	va_list args;
	va_start(args, format);
	vsprintf_s(line_buf, sizeof(line_buf), format, args);
	va_end(args);

	printf_s(line_buf);
	fprintf_s(out_asm_file, line_buf);
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
	Byte byte1 = data[0];
	Byte byte2 = data[1];
	uint32 bytes_read = 2;

	bool8 d = (byte1 >> 1) & 1;
	bool8 w = byte1 & 1;

	uint8 mod_field = byte2 >> 6;
	uint8 reg_field = (byte2 >> 3) & 0b111;
	uint8 rm_field = byte2 & 0b111;

	const char* reg_s = register_name_lookup[reg_field][w];
	char rm_s[32];

	// Register Mode
	if (mod_field == 0b11)
	{
		sprintf_s(rm_s, sizeof(rm_s), "%s", register_name_lookup[rm_field][w]);
	}
	// Direct Address Mem Mode
	else if ((!mod_field) * (rm_field == 0b110))
	{
		sprintf_s(rm_s, sizeof(rm_s), "[%hu]", *((Word*)&data[bytes_read]));
		bytes_read += 2;
	}
	// Mem Mode w/o Displacement
	else if (!mod_field)
	{
		sprintf_s(rm_s, sizeof(rm_s), "[%s]", mem_mode_rm_field_reg_lookup[rm_field]);
	}
	// Mem Mode with Displacement
	else
	{
		int32 displacement_value;
		if (mod_field == 0b01)
			displacement_value = (int8)data[bytes_read];
		else
			displacement_value = *((int16*)&data[bytes_read]);

		bytes_read += mod_field;

		if (displacement_value)
		{
			const char* signs[2] = {"+", "-"};
			const char* sign = signs[displacement_value < 0];

			sprintf_s(rm_s, sizeof(rm_s), "[%s %s %i]", mem_mode_rm_field_reg_lookup[rm_field], sign, abs(displacement_value));
		}
		else
		{
			sprintf_s(rm_s, sizeof(rm_s), "[%s]", mem_mode_rm_field_reg_lookup[rm_field]);
		}
	}

	if (d)
		_print_out("mov %s, %s\n", reg_s, rm_s);
	else
		_print_out("mov %s, %s\n", rm_s, reg_s);

	return bytes_read;
}

// 0b1100011w(1) mod(2)000r/m(3) {disp-low} {disp-high} data-low {data-high}
static int32 _inst_mov_imm_to_mem_reg(Byte* data, uint32 remaining_size)
{
	Byte byte1 = data[0];
	Byte byte2 = data[1];
	uint32 bytes_read = 2;

	bool8 w = byte1 & 1;

	uint8 mod_field = byte2 >> 6;
	uint8 rm_field = byte2 & 0b111;

	char src_s[32];
	char dest_s[32];

	// Register Mode
	if (mod_field == 0b11)
	{
		sprintf_s(dest_s, sizeof(dest_s), "%s", register_name_lookup[rm_field][w]);
	}
	// Direct Address Mem Mode
	else if ((!mod_field) * (rm_field == 0b110))
	{
		sprintf_s(dest_s, sizeof(dest_s), "[%hu]", *((Word*)&data[bytes_read]));
		bytes_read += 2;
	}
	// Mem Mode w/o Displacement
	else if (!mod_field)
	{
		sprintf_s(dest_s, sizeof(dest_s), "[%s]", mem_mode_rm_field_reg_lookup[rm_field]);
	}
	// Mem Mode with Displacement
	else
	{
		int32 displacement_value;
		if (mod_field == 0b01)
			displacement_value = (int8)data[bytes_read];
		else
			displacement_value = *((int16*)&data[bytes_read]);

		bytes_read += mod_field;

		if (displacement_value)
		{
			const char* signs[2] = {"+", "-"};
			const char* sign = signs[displacement_value < 0];

			sprintf_s(dest_s, sizeof(dest_s), "[%s %s %i]", mem_mode_rm_field_reg_lookup[rm_field], sign, abs(displacement_value));
		}
		else
		{
			sprintf_s(dest_s, sizeof(dest_s), "[%s]", mem_mode_rm_field_reg_lookup[rm_field]);
		}
	}

	if(w)
	{
		Word imm = *((Word*)&data[bytes_read]);
		bytes_read += 2;
		sprintf_s(src_s, sizeof(src_s), "word %hu", imm);
	}
	else
	{
		Byte imm = data[bytes_read];
		bytes_read += 1;
		sprintf_s(src_s, sizeof(src_s), "byte %hhu", imm);
	}

	_print_out("mov %s, %s\n", dest_s, src_s);

	return bytes_read;
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
		_print_out("mov %s, %hu\n", register_name_lookup[reg_field][w], imm);
		return 3;
	}
	else
	{
		Byte imm = data[1];
		_print_out("mov %s, %hhu\n", register_name_lookup[reg_field][w], imm);
		return 2;
	}
}

// 0b1010000w(1) addr-low addr-high
static int32 _inst_mov_mem_to_acc(Byte* data, uint32 remaining_size)
{
	Byte byte1 = data[0];

	bool8 w = byte1 & 1;
	
	if (w)
	{
		Word imm = *((Word*)&data[1]);
		_print_out("mov ax, [%hu]\n", imm);
	}
	else
	{
		Byte imm = data[1];
		_print_out("mov al, [%hhu]\n", imm);
	}

	return 3;
}

// 0b1010001w(1) addr-low addr-high
static int32 _inst_mov_acc_to_mem(Byte* data, uint32 remaining_size)
{
	Byte byte1 = data[0];

	bool8 w = byte1 & 1;
	
	if (w)
	{
		Word imm = *((Word*)&data[1]);
		_print_out("mov [%hu], ax\n", imm);
	}
	else
	{
		Byte imm = data[1];
		_print_out("mov [%hhu], al\n", imm);
	}

	return 3;
}

static void _fill_instruction_lookups()
{
	for (uint32 i = 0b10001000; i <= 0b10001011; i++) primary_instruction_lookup[i] = _inst_mov_mem_reg_transfer;
	for (uint32 i = 0b11000110; i <= 0b11000111; i++) primary_instruction_lookup[i] = _inst_mov_imm_to_mem_reg;
	for (uint32 i = 0b10110000; i <= 0b10111111; i++) primary_instruction_lookup[i] = _inst_mov_imm_to_reg;
	for (uint32 i = 0b10100000; i <= 0b10100001; i++) primary_instruction_lookup[i] = _inst_mov_mem_to_acc;
	for (uint32 i = 0b10100010; i <= 0b10100011; i++) primary_instruction_lookup[i] = _inst_mov_acc_to_mem;
}