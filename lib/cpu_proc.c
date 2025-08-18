#include <cpu.h>
#include <emu.h>
#include <bus.h>
#include <stack.h>

//processes CPU instructions...

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

static bool is_16_bit(reg_type rt) {
    return rt >= RT_AF;
}

static void proc_ld(cpu_context *context) {
    if (context->dest_is_mem) {
        //LD (BC), A for instance...

        if (is_16_bit(context->cur_inst->reg_2)) {
            //if 16 bit register...
            emu_cycles(1);
            bus_write16(context->mem_dest, context->fetch_data);
        } else {
            bus_write(context->mem_dest, context->fetch_data);
        }

        emu_cycles(1);

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
        bus_write(context->mem_dest, context->regs.a);
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
    char rel = (char)(context->fetch_data & 0xFF);
    u16 addr = context->regs.pc + rel;
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

static void proc_inc(cpu_context *context) {
    u16 val = cpu_read_reg(context->cur_inst->reg_1) + 1;

    if (is_16_bit(context->cur_inst->reg_1)) {
        emu_cycles(1);
    }

    if (context->cur_inst->reg_1 == RT_HL && context->cur_inst->mode == AM_MR) {
        val = bus_read(cpu_read_reg(RT_HL)) + 1;
        val &= 0xFF;
        bus_write(cpu_read_reg(RT_HL), val);
    } else {
        cpu_set_reg(context->cur_inst->reg_1, val);
        val = cpu_read_reg(context->cur_inst->reg_1);
    }

    if ((context->cur_opcode & 0x03) == 0x03) {
        return;
    }

    cpu_set_flags(context, val == 0, 0, (val & 0x0F) == 0, -1);
}

static void proc_dec(cpu_context *context) {
    u16 val = cpu_read_reg(context->cur_inst->reg_1) - 1;

    if (is_16_bit(context->cur_inst->reg_1)) {
        emu_cycles(1);
    }

    if (context->cur_inst->reg_1 == RT_HL && context->cur_inst->mode == AM_MR) {
        val = bus_read(cpu_read_reg(RT_HL)) - 1;
        bus_write(cpu_read_reg(RT_HL), val);
    } else {
        cpu_set_reg(context->cur_inst->reg_1, val);
        val = cpu_read_reg(context->cur_inst->reg_1);
    }

    if ((context->cur_opcode & 0x0B) == 0x0B) {
        return;
    }

    cpu_set_flags(context, val == 0, 1, (val & 0x0F) == 0x0F, -1);
}

static void proc_sub(cpu_context *context) {
    u16 val = cpu_read_reg(context->cur_inst->reg_1) - context->fetch_data;

    int z = val == 0;
    int h = ((int)cpu_read_reg(context->cur_inst->reg_1) & 0xF) - ((int)context->fetch_data & 0xF) < 0;
    int c = ((int)cpu_read_reg(context->cur_inst->reg_1)) - ((int)context->fetch_data) < 0;

    cpu_set_reg(context->cur_inst->reg_1, val);
    cpu_set_flags(context, z, 1, h, c);
}

// subtract with carry
static void proc_sbc(cpu_context *context) {
    u8 val = context->fetch_data + CPU_FLAG_C;

    int z = cpu_read_reg(context->cur_inst->reg_1) - val == 0;

    int h = ((int)cpu_read_reg(context->cur_inst->reg_1) & 0xF) 
        - ((int)context->fetch_data & 0xF) - ((int)CPU_FLAG_C) < 0;
    int c = ((int)cpu_read_reg(context->cur_inst->reg_1)) 
        - ((int)context->fetch_data) - ((int)CPU_FLAG_C) < 0;

    cpu_set_reg(context->cur_inst->reg_1, cpu_read_reg(context->cur_inst->reg_1) - val);
    cpu_set_flags(context, z, 1, h, c);
}

static void proc_adc(cpu_context *context) {
    u16 u = context->fetch_data;
    u16 a = context->regs.a;
    u16 c = CPU_FLAG_C;

    context->regs.a = (a + u + c) & 0xFF;

    cpu_set_flags(context, context->regs.a == 0, 0, 
        (a & 0xF) + (u & 0xF) + c > 0xF,
        a + u + c > 0xFF);
}

static void proc_add(cpu_context *context) {
    u32 val = cpu_read_reg(context->cur_inst->reg_1) + context->fetch_data;

    bool is_16bit = is_16_bit(context->cur_inst->reg_1);

    if (is_16bit) {
        emu_cycles(1);
    }

    if (context->cur_inst->reg_1 == RT_SP) {
        val = cpu_read_reg(context->cur_inst->reg_1) + (char)context->fetch_data;
    }

    int z = (val & 0xFF) == 0;
    int h = (cpu_read_reg(context->cur_inst->reg_1) & 0xF) + (context->fetch_data & 0xF) >= 0x10;
    int c = (int)(cpu_read_reg(context->cur_inst->reg_1) & 0xFF) + (int)(context->fetch_data & 0xFF) >= 0x100;

    if (is_16bit) {
        z = -1;
        h = (cpu_read_reg(context->cur_inst->reg_1) & 0xFFF) + (context->fetch_data & 0xFFF) >= 0x1000;
        u32 n = ((u32)cpu_read_reg(context->cur_inst->reg_1)) + ((u32)context->fetch_data);
        c = n >= 0x10000;
    }

    if (context->cur_inst->reg_1 == RT_SP) {
        z = 0;
        h = (cpu_read_reg(context->cur_inst->reg_1) & 0xF) + (context->fetch_data & 0xF) >= 0x10;
        c = (int)(cpu_read_reg(context->cur_inst->reg_1) & 0xFF) + (int)(context->fetch_data & 0xFF) > 0x100;
    }

    cpu_set_reg(context->cur_inst->reg_1, val & 0xFFFF);
    cpu_set_flags(context, z, 0, h, c);
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
    [IN_DEC] = proc_dec,
    [IN_INC] = proc_inc,
    [IN_ADD] = proc_add,
    [IN_ADC] = proc_adc,
    [IN_SUB] = proc_sub,
    [IN_SBC] = proc_sbc,
    [IN_RETI] = proc_reti,
    [IN_XOR] = proc_xor
};

IN_PROC inst_get_processor(in_type type) {
    return processors[type];
}