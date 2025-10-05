#pragma once
#include <stdint.h>
#include <stdarg.h>

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

#ifdef _MSC_VER
#pragma warning(disable : 4100)
#pragma warning(disable : 4101)
#pragma warning(disable : 4189)
#endif

#define array_count(arr) (sizeof(arr) / sizeof(arr[0]))

void print_out(const char* format, ...);

typedef enum
{
    OpType_none,
    #define inst_layout(mnemonic, ...) OpType_##mnemonic,
    #define inst_layout_alt(...)
    #include "instruction_layouts.inl"
    OpTypeCount
} OpType;
typedef uint8 OpTypeValue;

typedef struct
{
    OpTypeValue op_type;
    uint8 size;
    uint32 placeholder;
}
Instruction;
