#include <timer.h>
#include <interrupts.h>

static timer_context context = {0};

timer_context *timer_get_context() {
    return &context;
}

void timer_init() {
    context.div = 0xAC00;
}

void timer_tick() {
    u16 prev_div = context.div;

    context.div++;

    bool timer_update = false;

    switch(context.tac & (0b11)) {
        case 0b00:
            timer_update = (prev_div & (1 << 9)) && (!(context.div & (1 << 9)));
            break;
        case 0b01:
            timer_update = (prev_div & (1 << 3)) && (!(context.div & (1 << 3)));
            break;
        case 0b10:
            timer_update = (prev_div & (1 << 5)) && (!(context.div & (1 << 5)));
            break;
        case 0b11:
            timer_update = (prev_div & (1 << 7)) && (!(context.div & (1 << 7)));
            break;
    }

    if (timer_update && context.tac & (1 << 2)) {
        context.tima++;

        if (context.tima == 0xFF) {
            context.tima = context.tma;

            cpu_request_interrupt(IT_TIMER);
        }
    }
}

void timer_write(u16 address, u8 value) {
    switch(address) {
        case 0xFF04:
            //DIV
            context.div = 0;
            break;

        case 0xFF05:
            //TIMA
            context.tima = value;
            break;

        case 0xFF06:
            //TMA
            context.tma = value;
            break;

        case 0xFF07:
            //TAC
            context.tac = value;
            break;
    }
}

u8 timer_read(u16 address) {
    switch(address) {
        case 0xFF04:
            return context.div >> 8;
        case 0xFF05:
            return context.tima;
        case 0xFF06:
            return context.tma;
        case 0xFF07:
            return context.tac;
    }
}