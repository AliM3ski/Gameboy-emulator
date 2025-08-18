#include <cpu.h>
#include <bus.h>
#include <emu.h>

cpu_context context = {0};

void cpu_init() {
    context.regs.pc = 0x100;
    context.regs.a = 0x01;
}

static void fetch_instruction() {
    context.cur_opcode = bus_read(context.regs.pc++);
    context.cur_inst = instruction_by_opcode(context.cur_opcode);
}

void fetch_data();

static void execute() {
    IN_PROC proc = inst_get_processor(context.cur_inst->type);

    if (!proc) {
        NO_IMPL
    }

    proc(&context);
}

bool cpu_step() {
    
    if (!context.halted) {
        u16 pc = context.regs.pc;

        fetch_instruction();
        emu_cycles(1);
        fetch_data();

        char flags[16];
        sprintf(flags, "%c%c%c%c", 
            context.regs.f & (1 << 7) ? 'Z' : '-',
            context.regs.f & (1 << 6) ? 'N' : '-',
            context.regs.f & (1 << 5) ? 'H' : '-',
            context.regs.f & (1 << 4) ? 'C' : '-'
        );

        printf("%08lX - %04X: %-7s (%02X %02X %02X) A: %02X F: %s BC: %02X%02X DE: %02X%02X HL: %02X%02X\n", 
            emu_get_context()->ticks, pc, inst_name(context.cur_inst->type), context.cur_opcode,
            bus_read(pc + 1), bus_read(pc + 2), context.regs.a, flags, context.regs.b, context.regs.c, context.regs.d, context.regs.e, context.regs.h, context.regs.l);

        if (context.cur_inst == NULL) {
            printf("Unknown Instruction! %02X\n", context.cur_opcode);
            exit(-7);
        }
        execute();
    }

    return true;
}

u8 cpu_get_ie_register() {
    return context.ie_register;
}
void cpu_set_ie_register(u8 n) {
    context.ie_register = n;
}