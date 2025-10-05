#pragma once
#include "common.h"

void decoder2_initialize_lookup();
Instruction decoder2_decode_instruction(Byte* data, uint32 remaining_size);