#include "decoder2.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static char op_mnemonics[OpTypeCount][8] =
{
    [OpType_none] = "",
    #define inst_layout(mnemonic, ...) [OpType_##mnemonic] = #mnemonic,
    #define inst_layout_alt(...)
    #include "instruction_layouts.inl"
};

typedef enum
{
    IBitFieldType_None,
    IBitFieldType_Static,
    IBitFieldType_MOD,
    IBitFieldType_REG,
    IBitFieldType_RM,
    IBitFieldType_SR,
    IBitFieldType_Disp,
    IBitFieldType_Data,

    //IBitFieldType_HasDisp,
    //IBitFieldType_DispAlwaysW,
    //IBitFieldType_HasData,
    //IBitFieldType_WMakesDataW,
    //IBitFieldType_RMRegAlwaysW,
    //IBitFieldType_RelJUMPDisp,
    IBitFieldType_D,
    IBitFieldType_S,
    IBitFieldType_W,
    IBitFieldType_V,
    IBitFieldType_Z,

    IBitFieldTypeCount
} 
IBitFieldType;

typedef struct
{
    uint8 type;
    uint8 size;
    uint8 shift;
    uint8 value;
} 
IBitField;

typedef enum
{
    InstructionLayoutFlag_AlwaysHasDisp = 1 << 0,
    InstructionLayoutFlag_DispAlwaysW = 1 << 1,
    InstructionLayoutFlag_HasData = 1 << 2,
    InstructionLayoutFlag_DataWideIfW = 1 << 3,
    InstructionLayoutFlag_RMRegAlwaysW = 1 << 4,
    InstructionLayoutFlag_RelJUMPDisp = 1 << 5,
}
InstructionLayoutFlag;

typedef struct
{
    uint8 op_type;
    uint8 flags;
    IBitField fields[16];
}
InstructionLayout;

static InstructionLayout instruction_layouts[] =
{
    #define inst_layout(mnemonic, flags, ...) {OpType_##mnemonic, flags, __VA_ARGS__},
    #define inst_layout_alt(mnemonic, flags, ...) {OpType_##mnemonic, flags, __VA_ARGS__},
    #include "instruction_layouts.inl"
};

typedef struct
{
    bool8 use_first_index;
    uint8 layout_indices[8];
}
LookupEntry;

static LookupEntry instruction_layout_lookup[0xFF] = {0};

#define invalid_instruction_layout_id 0xFF

Instruction decoder2_decode_instruction(Byte* data, uint32 remaining_size)
{
    Instruction ret_inst = {.op_type = OpType_none};

    uint8 layout_index = invalid_instruction_layout_id;
    LookupEntry lookup_entry = instruction_layout_lookup[data[0]];
    if (lookup_entry.use_first_index)
        layout_index = lookup_entry.layout_indices[0];
    else
        layout_index = lookup_entry.layout_indices[(data[1] >> 3) & 0b111];

    if (layout_index == invalid_instruction_layout_id)
        return ret_inst;

    InstructionLayout layout = instruction_layouts[layout_index];
    ret_inst.op_type = layout.op_type;

    bool8 has_disp = ((layout.flags & InstructionLayoutFlag_AlwaysHasDisp) != 0);
    bool8 has_always_wide_disp = ((layout.flags & InstructionLayoutFlag_DispAlwaysW) != 0);
    bool8 has_data = ((layout.flags & InstructionLayoutFlag_HasData) != 0);
    bool8 has_wide_data_if_w = ((layout.flags & InstructionLayoutFlag_DataWideIfW) != 0);

    uint8 fields[IBitFieldTypeCount] = {0};
    for (uint8 layout_field_i = 0, bit_index = 0; layout_field_i < array_count(layout.fields) && layout.fields[layout_field_i].type != IBitFieldType_None; layout_field_i++)
    {
        IBitField layout_field = layout.fields[layout_field_i];
        uint8 byte = data[bit_index / 8];
        uint8 mask = 0xFF >> (8 - layout_field.size);

        fields[layout_field.type] = layout_field.value ? layout_field.value : ((byte >> layout_field.shift) & mask);
        bit_index += layout_field.size;
    }

    return ret_inst;
}

void decoder2_initialize_lookup()
{
    _Static_assert(array_count(instruction_layouts) < invalid_instruction_layout_id, "Instruction layout count exceeded invalid instruction id value.");
    memset(instruction_layout_lookup, 0xFFFFFFFF, sizeof(instruction_layout_lookup));

    for(uint8 layout_i = 0; layout_i < array_count(instruction_layouts); layout_i++)
    {
        InstructionLayout layout = instruction_layouts[layout_i];
        uint8 min_first_byte = 0;
        uint8 max_first_byte = 0;

        uint8 bit_index = 0;
        uint8 field_i = 0;
        while (field_i < array_count(layout.fields) && layout.fields[field_i].type != IBitFieldType_None)
        {
            IBitField field = layout.fields[field_i];
            if (bit_index < 8)
            {
                min_first_byte <<= field.size;
                max_first_byte <<= field.size;

                if (field.type == IBitFieldType_Static)
                {
                    min_first_byte |= field.value;
                    max_first_byte |= field.value;
                }
                else
                {
                    max_first_byte |= (0xFF >> (8 - field.size));
                }
            }

            instruction_layouts[layout_i].fields[field_i].shift = 8 - (bit_index % 8) - field.size;

            bit_index += field.size;
            field_i++;
        }
        // Check if instruction layout size conforms to multiple of bytes
        assert(bit_index % 8 == 0);

        bool8 use_secondary_lookup = {layout.fields[field_i + 1].type == IBitFieldType_Static};
        for (uint8 first_byte = min_first_byte; first_byte <= max_first_byte; first_byte++)
        {
            instruction_layout_lookup[first_byte].use_first_index = !use_secondary_lookup;
            uint8 sec_lookup_index = (use_secondary_lookup * layout.fields[field_i + 1].value);
            assert(sec_lookup_index < 8);
            instruction_layout_lookup[first_byte].layout_indices[sec_lookup_index] = layout_i;
        }
    }
}