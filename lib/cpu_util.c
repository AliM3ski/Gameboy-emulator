#include <cpu.h>
#include <bus.h>

extern cpu_context context;

u16 reverse(u16 n) {
    return ((n & 0xFF00) >> 8) | ((n & 0x00FF) << 8);
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

void cpu_set_reg(reg_type rt, u16 val) {
    switch(rt) {
        case RT_A: context.regs.a = val & 0xFF; break;
        case RT_F: context.regs.f = val & 0xFF; break;
        case RT_B: context.regs.b = val & 0xFF; break;
        case RT_C: {
             context.regs.c = val & 0xFF;
        } break;
        case RT_D: context.regs.d = val & 0xFF; break;
        case RT_E: context.regs.e = val & 0xFF; break;
        case RT_H: context.regs.h = val & 0xFF; break;
        case RT_L: context.regs.l = val & 0xFF; break;

        case RT_AF: *((u16 *)&context.regs.a) = reverse(val); break;
        case RT_BC: *((u16 *)&context.regs.b) = reverse(val); break;
        case RT_DE: *((u16 *)&context.regs.d) = reverse(val); break;
        case RT_HL: {
         *((u16 *)&context.regs.h) = reverse(val); 
         break;
        }

        case RT_PC: context.regs.pc = val; break;
        case RT_SP: context.regs.sp = val; break;
        case RT_NONE: break;
    }
}


u8 cpu_read_reg8(reg_type rt) {
    switch(rt) {
        case RT_A: return context.regs.a;
        case RT_F: return context.regs.f;
        case RT_B: return context.regs.b;
        case RT_C: return context.regs.c;
        case RT_D: return context.regs.d;
        case RT_E: return context.regs.e;
        case RT_H: return context.regs.h;
        case RT_L: return context.regs.l;
        case RT_HL: {
            return bus_read(cpu_read_reg(RT_HL));
        }
        default:
            printf("**ERR INVALID REG8: %d\n", rt);
            NO_IMPL
    }
}

void cpu_set_reg8(reg_type rt, u8 val) {
    switch(rt) {
        case RT_A: context.regs.a = val & 0xFF; break;
        case RT_F: context.regs.f = val & 0xFF; break;
        case RT_B: context.regs.b = val & 0xFF; break;
        case RT_C: context.regs.c = val & 0xFF; break;
        case RT_D: context.regs.d = val & 0xFF; break;
        case RT_E: context.regs.e = val & 0xFF; break;
        case RT_H: context.regs.h = val & 0xFF; break;
        case RT_L: context.regs.l = val & 0xFF; break;
        case RT_HL: bus_write(cpu_read_reg(RT_HL), val); break;
        default:
            printf("**ERR INVALID REG8: %d\n", rt);
            NO_IMPL
    }
}

cpu_registers *cpu_get_regs() {
    return &context.regs;
}

u8 cpu_get_int_flags() {
    return context.int_flags;
}

void cpu_set_int_flags(u8 value) {
    context.int_flags = value;
}