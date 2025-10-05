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
    
//#define Disp {IBitFieldType_HasDisp, 0, 0, 1}
//#define Addr {IBitFieldType_HasDisp, 0, 0, 1}, {IBitFieldType_DispAlwaysW, 0, 0, 1}
//#define Data {IBitFieldType_HasData, 0, 0, 1}
//#define DataIfW {IBitFieldType_WMakesDataW, 0, 0, 1}
//#define Flags(F) {F, 0, 0, 0, 1}

#define AlwaysHasDisp InstructionLayoutFlag_AlwaysHasDisp
#define HasAddr InstructionLayoutFlag_AlwaysHasDisp | InstructionLayoutFlag_DispAlwaysW
#define HasData InstructionLayoutFlag_HasData
#define DataWideIfW InstructionLayoutFlag_DataWideIfW

inst_layout(mov, 0, {B(100010), D, W, MOD, REG, RM})
inst_layout_alt(mov, HasData | DataWideIfW, {B(1100011), W, MOD, B(000), RM, ImpD(0)})
inst_layout_alt(mov, HasData | DataWideIfW, {B(1011), W, REG, ImpD(1)})
inst_layout_alt(mov, HasAddr, {B(1010000), W, ImpREG(0), ImpMOD(0), ImpRM(0b110), ImpD(1)})
inst_layout_alt(mov, HasAddr, {B(1010001), W, ImpREG(0), ImpMOD(0), ImpRM(0b110), ImpD(0)})
inst_layout_alt(mov, 0, {B(100011), D, B(0), MOD, B(0), SR, RM})

//inst_layout(push)
//inst_layout(pop)
//inst_layout(xchg)
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
//inst_layout(add)
//inst_layout(adc)
//inst_layout(inc)
//inst_layout(aaa)
//inst_layout(daa)
//inst_layout(sub)
//inst_layout(sbb)
//inst_layout(dec)
//inst_layout(neg)
//inst_layout(cmp)
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
//inst_layout(shr)
//inst_layout(sar)
//inst_layout(rol)
//inst_layout(ror)
//inst_layout(rcl)
//inst_layout(rcr)
//inst_layout(and)
//inst_layout(test)
//inst_layout(or)
//inst_layout(xor)
//inst_layout(rep)
//inst_layout(movs)
//inst_layout(cmps)
//inst_layout(scas)
//inst_layout(lods)
//inst_layout(stos)
//inst_layout(call)
//inst_layout(jmp)
//inst_layout(ret)
//inst_layout(je)
//inst_layout(jl)
//inst_layout(jle)
//inst_layout(jb)
//inst_layout(jbe)
//inst_layout(jp)
//inst_layout(jo)
//inst_layout(js)
//inst_layout(jne)
//inst_layout(jnl)
//inst_layout(jg)
//inst_layout(jnb)
//inst_layout(ja)
//inst_layout(jnp)
//inst_layout(jno)
//inst_layout(jns)
//inst_layout(loop)
//inst_layout(loopz)
//inst_layout(loopnz)
//inst_layout(jcxz)
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
//inst_layout(lock)
//inst_layout(segment)

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
  
#undef Disp
#undef Addr
#undef Data
#undef DataIfW
#undef Flags

#undef inst_layout
#undef inst_layout_alt