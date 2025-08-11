// processing cpu instructions

#include <cpu.h>
#include <emu.h>

static void proc_none(cpu_context *context) {
	printf("INVAILD INSTRUCTION!\n");
	exit(-7);
}

static void proc_nop(cpu_context *context) {
}

static void proc_di(cpu_context *context) {
	context->int_master_enabled = false;
}

static void proc_ld(cpu_context *context) {
	// TODO
}

void cpu_set_flags(cpu_context *context, char z, char n, char h, char c) {
	if (z != -1) {
		BIT_SET(context->regs.f, 7, z);
	}

	if (n != -1) {
		BIT_SET(context->regs.f, 6, n);
	}

	if (h != -1) {
		BIT_SET(context->regs.f, 5, h);
	}

	if (c != -1) {
		BIT_SET(context->regs.f, 4, c);
	}
}
static void proc_xor(cpu_context *context) {
	context->regs.a ^= context->fetch_data & 0xFF;
	cpu_set_flags(context, context->regs.a, 0, 0, 0);
}

static bool check_cond(cpu_context *context) {
	bool z = CPU_FLAG_Z;
	bool c = CPU_FLAG_C;

	switch(context->cur_inst->cond) {
		case CT_NONE: return true;
		case CT_C: return c;
		case CT_NC: return !c;
		case CT_Z: return z;
		case CT_NZ: return !z;

	}

	return false;
}

static void proc_jp(cpu_context *context) {
	if (check_cond(context)) {
		context->regs.pc = context->fetch_data;
		emu_cycles(1);

	}
}

static IN_PROC processors[] = {
	[IN_NONE] = proc_none, 
	[IN_NOP] = proc_nop, 
	[IN_LD] = proc_ld, // load
	[IN_JP] = proc_jp, // jump
	[IN_DI] = proc_di,
	[IN_XOR] = proc_xor
};


IN_PROC inst_get_processor(in_type type){
	return processors[type];
}