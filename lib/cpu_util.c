#include <cpu.h>

extern cpu_context context;

u16 reverse(u16 n) {
	return ((n  & 0xFF00 >> 8) | ((n & 0x00FF) << 8));
}
u16 cpu_read_reg(reg_type rt) {
    switch(rt) {
        case RT_A: return context.regs.a;
        case RT_F: return context.regs.f;
        case RT_B: return context.regs.b;
        case RT_C: return context.regs.c;
        case RT_D: return context.regs.d;
        case RT_E: return context.regs.e;
        case RT_H: return context.regs.h;
        case RT_L: return context.regs.l;

        case RT_AF: return reverse(*((u16 *)&context.regs.a));
        case RT_BC: return reverse(*((u16 *)&context.regs.b));
        case RT_DE: return reverse(*((u16 *)&context.regs.d));
        case RT_HL: return reverse(*((u16 *)&context.regs.h));

        case RT_PC: return context.regs.pc;
        case RT_SP: return context.regs.sp;
        default: return 0;
    }
}