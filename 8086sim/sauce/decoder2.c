#include "decoder2.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

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

static RegisterAccess register_lookup[8][2] =
{
    {{RegisterIndex_a, 0, 1}, {RegisterIndex_a, 0, 2}},
    {{RegisterIndex_c, 0, 1}, {RegisterIndex_c, 0, 2}},
    {{RegisterIndex_d, 0, 1}, {RegisterIndex_d, 0, 2}},
    {{RegisterIndex_b, 0, 1}, {RegisterIndex_b, 0, 2}},
    {{RegisterIndex_a, 1, 1}, {RegisterIndex_sp, 0, 2}},
    {{RegisterIndex_c, 1, 1}, {RegisterIndex_bp, 0, 2}},
    {{RegisterIndex_d, 1, 1}, {RegisterIndex_si, 0, 2}},
    {{RegisterIndex_b, 1, 1}, {RegisterIndex_di, 0, 2}},
};

Instruction decoder2_decode_instruction(Byte* read_ptr, uint32 remaining_size)
{
    Instruction ret_inst = {.op_type = OpType_none};

    uint8 layout_index = invalid_instruction_layout_id;
    LookupEntry lookup_entry = instruction_layout_lookup[read_ptr[0]];
    layout_index = lookup_entry.layout_indices[lookup_entry.use_first_index ? 0 : ((read_ptr[1] >> 3) & 0b111)];

    if (layout_index == invalid_instruction_layout_id)
        return ret_inst;

    InstructionLayout layout = instruction_layouts[layout_index];

    bool8 has_always_disp = ((layout.flags & InstructionLayoutFlag_AlwaysHasDisp) != 0);
    bool8 has_always_wide_disp = ((layout.flags & InstructionLayoutFlag_DispAlwaysW) != 0);
    bool8 has_data = ((layout.flags & InstructionLayoutFlag_HasData) != 0);
    bool8 has_wide_data_if_w = ((layout.flags & InstructionLayoutFlag_DataWideIfW) != 0);
    bool8 has_relative_jump_displacement = ((layout.flags & InstructionLayoutFlag_RelJUMPDisp) != 0);

    uint16 has_field_flags = 0;
    uint8 fields[IBitFieldTypeCount] = {0};
    uint8 bit_index = 0;
    for (uint8 layout_field_i = 0; layout_field_i < array_count(layout.fields) && layout.fields[layout_field_i].type != IBitFieldType_None; layout_field_i++)
    {
        IBitField layout_field = layout.fields[layout_field_i];
        uint8 byte = read_ptr[bit_index / 8];
        uint8 mask = 0xFF >> (8 - layout_field.size);

        fields[layout_field.type] = layout_field.value ? layout_field.value : ((byte >> layout_field.shift) & mask);
        has_field_flags |= (1 << layout_field.type);
        bit_index += layout_field.size;
    }
    ret_inst.size = (bit_index / 8);

    uint8 mod = fields[IBitFieldType_MOD];
    uint8 rm = fields[IBitFieldType_RM];
    uint8 w = fields[IBitFieldType_W];
    uint8 s = fields[IBitFieldType_S];
    uint8 d = fields[IBitFieldType_D];
    uint8 v = fields[IBitFieldType_V];

    bool8 has_direct_address = (mod == 0b00) && (rm == 0b110);
    bool8 has_disp = has_always_disp || (mod == 0b10) || (mod == 0b01) || has_direct_address;
    bool8 has_wide_disp = has_always_wide_disp || (mod == 0b10) || has_direct_address;
    bool8 has_wide_data = has_wide_data_if_w && (!s) && w; 

    int16 disp = 0;
    uint8 disp_size = has_disp + (has_wide_disp * has_disp);
    memcpy(&disp, &read_ptr[ret_inst.size], disp_size);

    int16 data = 0;
    uint8 data_size = has_data + (has_wide_data * has_data);
    memcpy(&data, &read_ptr[ret_inst.size], data_size);

    ret_inst.size += disp_size;
    ret_inst.size += data_size;
    ret_inst.op_type = layout.op_type;
    ret_inst.flags = 0;
    ret_inst.address = 0;
    if (w)
        ret_inst.flags |= InstructionFlag_Wide;

    InstructionOperand* reg_operand = &ret_inst.operands[!d];
    InstructionOperand* mod_operand = &ret_inst.operands[d];

    if (has_field_flags & (1 << IBitFieldType_SR))
    {
        reg_operand->operand_type = InstructionOperandType_Register;
        reg_operand->register_access.reg_index = RegisterIndex_es + (fields[IBitFieldType_SR] & 0b11);
        reg_operand->register_access.count = 2;
    }

    if (has_field_flags & (1 << IBitFieldType_REG))
    {
        *reg_operand = (InstructionOperand){ .operand_type = InstructionOperandType_Register, .register_access = register_lookup[fields[IBitFieldType_REG]][w] };
    }

    if (has_field_flags & (1 << IBitFieldType_MOD))
    {
        if (mod == 0b11)
        {
            *mod_operand = (InstructionOperand){ .operand_type = InstructionOperandType_Register, .register_access = register_lookup[rm][w] };
        }
        else
        {
            mod_operand->operand_type = InstructionOperandType_Memory;
            mod_operand->effective_address.segment = 0;
            mod_operand->effective_address.displacement = disp;

            if (mod == 0b00 && rm == 0b110)
                mod_operand->effective_address.base = EffectiveAddressBase_direct;
            else
                mod_operand->effective_address.base = rm + 1;
        }
    }

    InstructionOperand* last_operand = &ret_inst.operands[(ret_inst.operands[0].operand_type != InstructionOperandType_None)];

    if (has_relative_jump_displacement)
    {
        last_operand->operand_type = InstructionOperandType_Immediate;
        last_operand->immediate_signed = disp + ret_inst.size;
    }

    if (has_data)
    {
        last_operand->operand_type = InstructionOperandType_Immediate;
        last_operand->immediate_unsigned = data;
    }

    if (has_field_flags & (1 << IBitFieldType_V))
    {
        if (v)
        {
            last_operand->operand_type = InstructionOperandType_Register;
            last_operand->register_access.reg_index = RegisterIndex_c;
            last_operand->register_access.offset = 0;
            last_operand->register_access.count = 1;
        }
        else
        {
            last_operand->operand_type = InstructionOperandType_Immediate;
            last_operand->immediate_signed = 1;
        }
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