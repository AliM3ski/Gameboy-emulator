#include <cpu.h>
#include <stack.h>
#include <interrupts.h>

void int_handle(cpu_context *context, u16 address) {
    stack_push16(context->regs.pc);
    context->regs.pc = address;
}

bool int_check(cpu_context *context, u16 address, interrupt_type it) {
    if (context->int_flags & it && context->ie_register & it) {
        int_handle(context, address);
        context->int_flags &= ~it;
        context->halted = false;
        context->int_master_enabled = false;

        return true;
    }

    return false;
}

void cpu_handle_interrupts(cpu_context *context) {
    if (int_check(context, 0x40, IT_VBLANK)) {

    } else if (int_check(context, 0x48, IT_LCD_STAT)) {

    } else if (int_check(context, 0x50, IT_TIMER)) {

    }  else if (int_check(context, 0x58, IT_SERIAL)) {

    }  else if (int_check(context, 0x60, IT_JOYPAD)) {

    } 
}