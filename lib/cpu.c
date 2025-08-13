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
        fetch_data();

        printf("%04X: %-7s (%02X %02X %02X) A: %02X B: %02X C: %02X\n", 
            pc, inst_name(context.cur_inst->type), context.cur_opcode,
            bus_read(pc + 1), bus_read(pc + 2), context.regs.a, context.regs.b, context.regs.c);

        if (context.cur_inst == NULL) {
            printf("Unknown Instruction! %02X\n", context.cur_opcode);
            exit(-7);
        }

        execute();
    }

    return true;
}