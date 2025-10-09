#pragma once
#include "common.h"

void decoder_init();
Instruction decoder_decode_instruction(Byte* data, uint32 remaining_size);