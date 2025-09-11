#include <dma.h>
#include <ppu.h>
#include <bus.h>

//For Windows
#include <pthread.h>
#include <unistd.h>

typedef struct {
    bool active;
    u8 byte;
    u8 value;
    u8 start_delay;
} dma_context;

static dma_context context;

void dma_start(u8 start) {
    context.active = true;
    context.byte = 0;
    context.start_delay = 2;
    context.value = start;
}


void dma_tick() {
    if (!context.active) {
        return;
    }

    if (context.start_delay) {
        context.start_delay--;
        return;
    }

    ppu_oam_write(context.byte, bus_read((context.value * 0x100) + context.byte));

    context.byte++;

    context.active = context.byte < 0xA0;


}

bool dma_transferring() {
    return context.active;
}