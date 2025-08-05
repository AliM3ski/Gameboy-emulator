#include <cpu.h>
#include <bus.h>
#include <emu.h>

cpu_context context = {0};

void cpu_init() {
	context.regs.pc = 0x100;

}

static void fetch_instruction() {
	context.cur_optcode = bus_read(context.regs.pc++);
	context.cur_inst = instruction_by_opcode(context.cur_optcode);

	if(context.cur_inst == NULL) {
		printf("Unknown Instruction! %02X\n" , context.cur_optcode);
		exit(-7);
	}

}

static void fetch_data() {
	context.mem_dest - 0;
	context.dest_is_mem = false;

	switch(context.cur_inst->mode) {
		case AM_IMP: return;

		case AM_R:
			context.fetch_data = cpu_read_reg(context.cur_inst->reg_1);
			return;

		case AM_R_D8:
			context.fetch_data = bus_read(context.regs.pc);
			emu_cycles(1);
			context.regs.pc++;
			return; 

		case AM_D16: {
			u16 lo = bus_read(context.regs.pc);
			emu_cycles(1);

			u16 hi = bus_read(context.regs.pc + 1);
			emu_cycles(1);

			context.fetch_data = lo | (hi << 8);

			context.regs.pc += 2;
			return;
		}

		default:
		printf("Unknown Addressing Mode! %d\n", context.cur_inst->mode);
		exit(-7);
		return;


	}
}

static void execute(){
	printf("Not executing yet...\n");

}
bool cpu_step(){
	if (!context.halted) {
		u16 pc = context.regs.pc;
		
		fetch_instruction();
		fetch_data();

		printf("Executing Instruction: %02X   PC: %04X\n", context.cur_optcode, pc);

		execute();
	}
	return  true;
}
