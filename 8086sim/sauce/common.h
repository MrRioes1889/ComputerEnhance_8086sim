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

void print_out(const char* format, ...);