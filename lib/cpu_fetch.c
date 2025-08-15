#include <cpu.h>
#include <bus.h>
#include <emu.h>

extern cpu_context context;

void fetch_data() {
	context.mem_dest = 0;
	context.dest_is_mem = false;

	switch(context.cur_inst->mode) {
		case AM_IMP: return;

		case AM_R:
			context.fetch_data = cpu_read_reg(context.cur_inst->reg_1);
			return;
		case AM_R_R:
			context.fetch_data = cpu_read_reg(context.cur_inst->reg_2);
			return;

		case AM_R_D8:
			context.fetch_data = bus_read(context.regs.pc);
			emu_cycles(1);
			context.regs.pc++;
			return; 

		case AM_R_D16: 
		case AM_D16: {
			u16 lo = bus_read(context.regs.pc);
			emu_cycles(1);

			u16 hi = bus_read(context.regs.pc + 1);
			emu_cycles(1);

			context.fetch_data = lo | (hi << 8);

			context.regs.pc += 2;
			return;
		}
		case AM_MR_R:
			context.fetch_data = cpu_read_reg(context.cur_inst->reg_2);
			context.mem_dest = cpu_read_reg(context.cur_inst->reg_1);
			context.dest_is_mem = true;

			if (context.cur_inst->reg_1 == RT_C) {
				context.mem_dest |= 0xFF00;
			}

			return;

		case AM_R_MR: {
			u16 addr = cpu_read_reg(context.cur_inst->reg_2);

			if (context.cur_inst->reg_2 == RT_C) {
				addr |= 0xFF00;
			}

			context.fetch_data = bus_read(addr);
			emu_cycles(1);
		} return;

		// incrementing
		case AM_R_HLI:
			context.fetch_data = bus_read(cpu_read_reg(context.cur_inst->reg_2));
			emu_cycles(1);
			cpu_set_reg(RT_HL, cpu_read_reg(RT_HL) + 1);
			return;
		// decrementing
		case AM_R_HLD:
			context.fetch_data = bus_read(cpu_read_reg(context.cur_inst->reg_2));
			emu_cycles(1);
			cpu_set_reg(RT_HL, cpu_read_reg(RT_HL) - 1);
			return;

		case AM_HLI_R:
			context.fetch_data = cpu_read_reg(context.cur_inst->reg_2);
			context.mem_dest = cpu_read_reg(context.cur_inst->reg_1);
			context.dest_is_mem = true;
			cpu_set_reg(RT_HL, cpu_read_reg(RT_HL) + 1);
			return;

		case AM_HLD_R:
			context.fetch_data = cpu_read_reg(context.cur_inst->reg_2);
			context.mem_dest = cpu_read_reg(context.cur_inst->reg_1);
			context.dest_is_mem = true;
			cpu_set_reg(RT_HL, cpu_read_reg(RT_HL) - 1);
			return;
		
		case AM_R_A8:
			context.fetch_data = bus_read(context.regs.pc);
			emu_cycles(1);
			context.regs.pc++;
			return;

		case AM_A8_R:
			context.mem_dest = bus_read(context.regs.pc) | 0xFF00;
			context.dest_is_mem = true;
			emu_cycles(1);
			context.regs.pc++;
			return;
		
		case AM_HL_SPR:
			context.fetch_data = bus_read(context.regs.pc);
			emu_cycles(1);
			context.regs.pc++;
			return;

		case AM_D8:
			context.fetch_data = bus_read(context.regs.pc);
			emu_cycles(1);
			context.regs.pc++;
			return;

		case AM_A16_R:
		case AM_D16_R: {
			u16 lo = bus_read(context.regs.pc);
			emu_cycles(1);

			u16 hi = bus_read(context.regs.pc + 1);
			emu_cycles(1);

			context.mem_dest = lo | (hi << 8);
			context.dest_is_mem = true;

			context.regs.pc += 2;
			context.fetch_data = cpu_read_reg(context.cur_inst->reg_2);
		
		} return;

		case AM_MR_D8:
			context.fetch_data = bus_read(context.regs.pc);
			emu_cycles(1);
			context.regs.pc++;
			context.mem_dest = cpu_read_reg(context.cur_inst->reg_1);
			context.dest_is_mem = true;
			return;

		case AM_MR:
			context.mem_dest = cpu_read_reg(context.cur_inst->reg_1);
			context.dest_is_mem = true;
			context.fetch_data = bus_read(cpu_read_reg(context.cur_inst->reg_1));
			emu_cycles(1);
			return;
		
		 case AM_R_A16: {
            u16 lo = bus_read(context.regs.pc);
            emu_cycles(1);

            u16 hi = bus_read(context.regs.pc + 1);
            emu_cycles(1);

            u16 addr = lo | (hi << 8);

            context.regs.pc += 2;
            context.fetch_data = bus_read(addr);
            emu_cycles(1);

            return;
        }



		default:
		printf("Unknown Addressing Mode! %d\n", context.cur_inst->mode);
		exit(-7);
		return;


	}
}
