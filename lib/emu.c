#include <stdio.h>
#include <emu.h>
#include <cart.h>
#include <cpu.h>
#include <ui.h>
#include <timer.h>
#include <dma.h>
#include <ppu.h>
#include <audio.h>    
#include <apu.h>  

//TODO Add Windows Alternative...
#include <pthread.h>
#include <unistd.h>

static emu_context context;

emu_context *emu_get_context() {
    return &context;
}

void *cpu_run(void *p) {
    timer_init();
    cpu_init();
    ppu_init();

    context.running = true;
    context.paused = false;
    context.ticks = 0;

    while(context.running) {
        if (context.paused) {
            usleep(10000); // sleep 10ms
            continue;
        }

        if (!cpu_step()) {
            printf("CPU Stopped\n");
            return 0;
        }
    }

    return 0;
}

int emu_run(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: emu <rom_file>\n");
        return -1;
    }

    if (!cart_load(argv[1])) {
        printf("Failed to load ROM file: %s\n", argv[1]);
        return -2;
    }

    printf("Cart loaded..\n");

    ui_init();
    apu_init();


    pthread_t t1;
    if (pthread_create(&t1, NULL, cpu_run, NULL)) {
        fprintf(stderr, "FAILED TO START MAIN CPU THREAD!\n");
        return -1;
    }

    u32 prev_frame = 0;

    while(!context.die) {
        usleep(1000);
        ui_handle_events();

        if (prev_frame != ppu_get_context()->current_frame) {
            ui_update();
        }

        prev_frame = ppu_get_context()->current_frame;
    }

    apu_quit();
    return 0;
}

void emu_cycles(int cpu_cycles) {
    for (int i=0; i<cpu_cycles; i++) {
        for (int n=0; n<4; n++) {
            context.ticks++;
            timer_tick();
            ppu_tick();
            apu_step(1); 
        }

        dma_tick();
    }
}
