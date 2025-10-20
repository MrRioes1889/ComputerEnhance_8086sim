#ifndef inst_layout
#define inst_layout(...)
#endif

#ifndef inst_layout_alt
#define inst_layout_alt(...)
#endif

#define B(Bits) {IBitFieldType_Static, sizeof(#Bits)-1, 0, 0b##Bits}
#define D {IBitFieldType_D, 1}
#define S {IBitFieldType_S, 1}
#define W {IBitFieldType_W, 1}
#define V {IBitFieldType_V, 1}
#define Z {IBitFieldType_Z, 1}

#define XXX {IBitFieldType_Data, 3}
#define YYY {IBitFieldType_Data, 3}
#define RM {IBitFieldType_RM, 3}
#define MOD {IBitFieldType_MOD, 2}
#define REG {IBitFieldType_REG, 3}
#define SR {IBitFieldType_SR, 2}

#define ImpW(Value) {IBitFieldType_W, 0, 0, Value}
#define ImpREG(Value) {IBitFieldType_REG, 0, 0, Value}
#define ImpMOD(Value) {IBitFieldType_MOD, 0, 0, Value}
#define ImpRM(Value) {IBitFieldType_RM, 0, 0, Value}
#define ImpD(Value) {IBitFieldType_D, 0, 0, Value}
#define ImpS(Value) {IBitFieldType_S, 0, 0, Value}

#define F_AlwaysHasDisp InstructionLayoutFlag_AlwaysHasDisp
#define F_HasAddr InstructionLayoutFlag_AlwaysHasDisp | InstructionLayoutFlag_DispAlwaysW
#define F_HasData InstructionLayoutFlag_HasData
#define F_DataWideIfW InstructionLayoutFlag_DataWideIfW
#define F_RelativeJumpDisp InstructionLayoutFlag_RelJUMPDisp | InstructionLayoutFlag_AlwaysHasDisp

inst_layout(mov, 0, {B(100010), D, W, MOD, REG, RM})
inst_layout_alt(mov, F_HasData | F_DataWideIfW, {B(1100011), W, MOD, B(000), RM, ImpD(0)})
inst_layout_alt(mov, F_HasData | F_DataWideIfW, {B(1011), W, REG, ImpD(1)})
inst_layout_alt(mov, F_HasAddr, {B(1010000), W, ImpREG(0b000), ImpMOD(0), ImpRM(0b110), ImpD(1)})
inst_layout_alt(mov, F_HasAddr, {B(1010001), W, ImpREG(0b000), ImpMOD(0), ImpRM(0b110), ImpD(0)})
inst_layout_alt(mov, 0, {B(100011), D, B(0), MOD, B(0), SR, RM})

//inst_layout(push)
//inst_layout(pop)

inst_layout(xchg, 0, {B(1000011), W, MOD, REG, RM, ImpD(1)})
inst_layout_alt(xchg, 0, {B(10010), REG, ImpMOD(0b11), ImpW(1), ImpRM(0)})

//inst_layout(in)
//inst_layout(out)
//inst_layout(xlat)
//inst_layout(lea)
//inst_layout(lds)
//inst_layout(les)
//inst_layout(lahf)
//inst_layout(sahf)
//inst_layout(pushf)
//inst_layout(popf)

inst_layout(add, 0, {B(000000), D, W, MOD, REG, RM})
inst_layout_alt(add, F_HasData | F_DataWideIfW, {B(100000), S, W, MOD, B(000), RM})
inst_layout_alt(add, F_HasData | F_DataWideIfW, {B(0000010), W, ImpREG(0b000), ImpD(1)})

//inst_layout(adc)

inst_layout(inc, 0, {B(1111111), W, MOD, B(000), RM, ImpD(1)})
inst_layout_alt(inc, 0, {B(01000), REG, ImpW(1), ImpD(1)})

//inst_layout(aaa)
//inst_layout(daa)

inst_layout(sub, 0, {B(001010), D, W, MOD, REG, RM})
inst_layout_alt(sub, F_HasData | F_DataWideIfW, {B(100000), S, W, MOD, B(101), RM})
inst_layout_alt(sub, F_HasData | F_DataWideIfW, {B(0010110), W, ImpREG(0b000), ImpD(1)})

//inst_layout(sbb)
//inst_layout(dec)

inst_layout(dec, 0, {B(1111111), W, MOD, B(001), RM, ImpD(1)})
inst_layout_alt(dec, 0, {B(01001), REG, ImpW(1), ImpD(1)})

//inst_layout(neg)

inst_layout(cmp, 0, {B(001110), D, W, MOD, REG, RM})
inst_layout_alt(cmp, F_HasData | F_DataWideIfW, {B(100000), S, W, MOD, B(111), RM})
inst_layout_alt(cmp, F_HasData | F_DataWideIfW, {B(0011110), W, ImpREG(0b000), ImpD(1)})

//inst_layout(aas)
//inst_layout(das)
//inst_layout(mul)
//inst_layout(imul)
//inst_layout(aam)
//inst_layout(div)
//inst_layout(idiv)
//inst_layout(aad)
//inst_layout(cbw)
//inst_layout(cwd)
//inst_layout(not)
//inst_layout(shl)
inst_layout(shr, 0, {B(110100), V, W, MOD, B(101), RM})
//inst_layout(sar)
//inst_layout(rol)
//inst_layout(ror)
//inst_layout(rcl)
//inst_layout(rcr)
//inst_layout(and)

inst_layout(test, 0, {B(1000010), W, MOD, REG, RM})
inst_layout_alt(test, F_HasData | F_DataWideIfW, {B(1111011), W, MOD, B(000), RM, ImpD(0)})
inst_layout_alt(test, F_HasData | F_DataWideIfW, {B(1010100), W, ImpREG(0b000), ImpD(1)})

//inst_layout(or)

inst_layout(xor, 0, {B(001100), D, W, MOD, REG, RM})
inst_layout_alt(xor, F_HasData | F_DataWideIfW, {B(1000000), W, MOD, B(110), RM, ImpD(0)})
inst_layout_alt(xor, F_HasData | F_DataWideIfW, {B(0011010), W, ImpREG(0b000), ImpD(1)})

inst_layout(rep, 0, {B(1111001), Z})
//inst_layout(movs)
//inst_layout(cmps)
//inst_layout(scas)
//inst_layout(lods)
//inst_layout(stos)
//inst_layout(call)
//inst_layout(jmp)

inst_layout(ret, 0, {B(11000011)})
inst_layout_alt(ret, F_HasData | F_DataWideIfW, {B(11000010), ImpW(1)})
inst_layout(retf, 0, {B(11001011)})
inst_layout_alt(retf, F_HasData | F_DataWideIfW, {B(11001010), ImpW(1)})

inst_layout(jz, F_RelativeJumpDisp, {B(01110100)})
inst_layout(jl, F_RelativeJumpDisp, {B(01111100)})
inst_layout(jle, F_RelativeJumpDisp, {B(01111110)})
inst_layout(jb, F_RelativeJumpDisp, {B(01110010)})
inst_layout(jbe, F_RelativeJumpDisp, {B(01110110)})
inst_layout(jp, F_RelativeJumpDisp, {B(01111010)})
inst_layout(jo, F_RelativeJumpDisp, {B(01110000)})
inst_layout(js, F_RelativeJumpDisp, {B(01111000)})
inst_layout(jnz, F_RelativeJumpDisp, {B(01110101)})
inst_layout(jnl, F_RelativeJumpDisp, {B(01111101)})
inst_layout(jg, F_RelativeJumpDisp, {B(01111111)})
inst_layout(jnb, F_RelativeJumpDisp, {B(01110011)})
inst_layout(ja, F_RelativeJumpDisp, {B(01110111)})
inst_layout(jnp, F_RelativeJumpDisp, {B(01111011)})
inst_layout(jno, F_RelativeJumpDisp, {B(01110001)})
inst_layout(jns, F_RelativeJumpDisp, {B(01111001)})
inst_layout(loop, F_RelativeJumpDisp, {B(11100010)})
inst_layout(loopz, F_RelativeJumpDisp, {B(11100001)})
inst_layout(loopnz, F_RelativeJumpDisp, {B(11100000)})
inst_layout(jcxz, F_RelativeJumpDisp, {B(11100011)})

//inst_layout(int)
//inst_layout(int3)
//inst_layout(into)
//inst_layout(iret)
//inst_layout(clc)
//inst_layout(cmc)
//inst_layout(stc)
//inst_layout(cld)
//inst_layout(std)
//inst_layout(cli)
//inst_layout(sti)
//inst_layout(hlt)
//inst_layout(wait)
//inst_layout(esc)
inst_layout(lock, 0, {B(11110000)})
inst_layout(segment, 0, {B(001), SR, B(110)})

#undef B
#undef D
#undef S
#undef W
#undef V
#undef Z

#undef XXX
#undef YYY
#undef RM
#undef MOD
#undef REG
#undef SR
   
#undef ImpW
#undef ImpREG
#undef ImpMOD
#undef ImpRM
#undef ImpD
#undef ImpS

#undef F_AlwaysHasDisp
#undef F_HasAddr
#undef F_HasData
#undef F_DataWideIfW
#undef F_RelativeJumpDisp

#undef inst_layout
#undef inst_layout_alt