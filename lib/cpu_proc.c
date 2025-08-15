#include <cpu.h>
#include <emu.h>
#include <bus.h>
#include <stack.h>

//processing CPU instructions

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

static void proc_none(cpu_context *context) {
    printf("INVALID INSTRUCTION!\n");
    exit(-7);
}

static void proc_nop(cpu_context *context) {

}

static void proc_di(cpu_context *context) {
    context->int_master_enabled = false;
}

static void proc_ld(cpu_context *context) {
    if (context->dest_is_mem) {
        //LD (BC), A for instance...

        if (context->cur_inst->reg_2 >= RT_AF) {
            //if 16 bit register...
            emu_cycles(1);
            bus_write16(context->mem_dest, context->fetch_data);
        } else {
            bus_write(context->mem_dest, context->fetch_data);
        }

        return;
    }

    if (context->cur_inst->mode == AM_HL_SPR) {
        u8 hflag = (cpu_read_reg(context->cur_inst->reg_2) & 0xF) + 
            (context->fetch_data & 0xF) >= 0x10;

        u8 cflag = (cpu_read_reg(context->cur_inst->reg_2) & 0xFF) + 
            (context->fetch_data & 0xFF) >= 0x100;

        cpu_set_flags(context, 0, 0, hflag, cflag);
        cpu_set_reg(context->cur_inst->reg_1, 
            cpu_read_reg(context->cur_inst->reg_2) + (char)context->fetch_data);

        return;
    }

    cpu_set_reg(context->cur_inst->reg_1, context->fetch_data);
}

static void proc_ldh(cpu_context *context) {
    if (context->cur_inst->reg_1 == RT_A) {
        cpu_set_reg(context->cur_inst->reg_1, bus_read(0xFF00 | context->fetch_data));
    } else {
        bus_write(0xFF00 | context->fetch_data, context->regs.a);
    }

    emu_cycles(1);
}

static void proc_xor(cpu_context *context) {
    context->regs.a ^= context->fetch_data & 0xFF;
    cpu_set_flags(context, context->regs.a == 0, 0, 0, 0);
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

static void goto_addr(cpu_context *context, u16 addr, bool pushpc) {
    if (check_cond(context)) {
        if (pushpc) {
            emu_cycles(2);
            stack_push16(context->regs.pc);
        }

        context->regs.pc = addr;
        emu_cycles(1);
    }
}

static void proc_jp(cpu_context *context) {
    goto_addr(context, context->fetch_data, false);
}

static void proc_jr(cpu_context *context) {
    char relative = (char)(context->fetch_data & 0xFF);
    u16 addr = context->regs.pc + relative;
    goto_addr(context, addr, false);
}

static void proc_call(cpu_context *context) {
    goto_addr(context, context->fetch_data, true);
}

static void proc_rst(cpu_context *context) {
    goto_addr(context, context->cur_inst->param, true);
}

static void proc_ret(cpu_context *context) {
    if (context->cur_inst->cond != CT_NONE) {
        emu_cycles(1);
    }

    if (check_cond(context)) {
        u16 lo = stack_pop();
        emu_cycles(1);
        u16 hi = stack_pop();
        emu_cycles(1);

        u16 n = (hi << 8) | lo;
        context->regs.pc = n;

        emu_cycles(1);
    }
}

// return from interupt
static void proc_reti(cpu_context *context) {
    context->int_master_enabled = true;
    proc_ret(context);
}

static void proc_pop(cpu_context *context) {
    u16 lo = stack_pop();
    emu_cycles(1);
    u16 hi = stack_pop();
    emu_cycles(1);

    u16 n = (hi << 8) | lo;

    cpu_set_reg(context->cur_inst->reg_1, n);

    if (context->cur_inst->reg_1 == RT_AF) {
        cpu_set_reg(context->cur_inst->reg_1, n & 0xFFF0);
    }
}

static void proc_push(cpu_context *context) {
    u16 hi = (cpu_read_reg(context->cur_inst->reg_1) >> 8) & 0xFF;
    emu_cycles(1);
    stack_push(hi);

    u16 lo = cpu_read_reg(context->cur_inst->reg_1) & 0xFF;
    emu_cycles(1);
    stack_push(lo);
    
    emu_cycles(1);
}

static IN_PROC processors[] = {
    [IN_NONE] = proc_none,
    [IN_NOP] = proc_nop,
    [IN_LD] = proc_ld,
    [IN_LDH] = proc_ldh,
    [IN_JP] = proc_jp,
    [IN_DI] = proc_di,
    [IN_POP] = proc_pop,
    [IN_PUSH] = proc_push,
    [IN_JR] = proc_jr,
    [IN_CALL] = proc_call,
    [IN_RET] = proc_ret,
    [IN_RST] = proc_rst,
    [IN_RETI] = proc_reti,
    [IN_XOR] = proc_xor
};

IN_PROC inst_get_processor(in_type type) {
    return processors[type];
}