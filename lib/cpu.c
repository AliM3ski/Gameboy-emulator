#include <cpu.h>
#include <bus.h>
#include <emu.h>
#include <interrupts.h>
#include <dbg.h>
#include <timer.h>

cpu_context context = {0};

#define CPU_DEBUG 0

void cpu_init() {
    context.regs.pc = 0x100;
    context.regs.sp = 0xFFFE;
    *((short *)&context.regs.a) = 0xB001;
    *((short *)&context.regs.b) = 0x1300;
    *((short *)&context.regs.d) = 0xD800;
    *((short *)&context.regs.h) = 0x4D01;
    context.ie_register = 0;
    context.int_flags = 0;
    context.int_master_enabled = false;
    context.enabling_ime = false;

    timer_get_context()->div = 0xABCC;
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

#if CPU_DEBUG == 1
        char flags[16];
        sprintf(flags, "%c%c%c%c", 
            context.regs.f & (1 << 7) ? 'Z' : '-',
            context.regs.f & (1 << 6) ? 'N' : '-',
            context.regs.f & (1 << 5) ? 'H' : '-',
            context.regs.f & (1 << 4) ? 'C' : '-'
        );

        char inst[16];
        inst_to_str(&context, inst);

        printf("%08lX - %04X: %-12s (%02X %02X %02X) A: %02X F: %s BC: %02X%02X DE: %02X%02X HL: %02X%02X\n", 
            emu_get_context()->ticks,
            pc, inst, context.cur_opcode,
            bus_read(pc + 1), bus_read(pc + 2), context.regs.a, flags, context.regs.b, context.regs.c,
            context.regs.d, context.regs.e, context.regs.h, context.regs.l);
#endif

        if (context.cur_inst == NULL) {
            printf("Unknown Instruction! %02X\n", context.cur_opcode);
            exit(-7);
        }

        dbg_update();
        dbg_print();

        execute();
    } else {
        //is halted...
        emu_cycles(1);

        if (context.int_flags) {
            context.halted = false;
        }
    }

    if (context.int_master_enabled) {
        cpu_handle_interrupts(&context);
        context.enabling_ime = false;
    }

    if (context.enabling_ime) {
        context.int_master_enabled = true;
    }

    return true;
}

u8 cpu_get_ie_register() {
    return context.ie_register;
}

void cpu_set_ie_register(u8 n) {
    context.ie_register = n;
}

void cpu_request_interrupt(interrupt_type t) {
    context.int_flags |= t;
}