#pragma once
#include "common.h"

void decoder_initialize_lookups();
int32 decoder_decode_instruction(Byte* data, uint32 remaining_size);