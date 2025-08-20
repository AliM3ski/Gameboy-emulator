#include <stdio.h>
#include <emu.h>
#include <cart.h>
#include <cpu.h>
#include <ui.h>

//TODO Add Windows Alternative...
#include <pthread.h>
#include <unistd.h>

/*
Emulator components:

Cartridge
CPU
Address Bus
PPU(pixel processing unit)
Timer

*/

static emu_context context;

emu_context *emu_get_context() {
	return &context;
}

void *cpu_run(void *p) {
    cpu_init();

    context.running = true;
    context.paused = false;
    context.ticks = 0;

    while(context.running) {
        if (context.paused) {
            delay(10);
            continue;
        }

        if (!cpu_step()) {
            printf("CPU Stopped\n");
            return 0;
        }

        context.ticks++;
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
    
    pthread_t t1;

    if (pthread_create(&t1, NULL, cpu_run, NULL)) {
        fprintf(stderr, "FAILED TO START MAIN CPU THREAD!\n");
        return -1;
    }

    while(!context.die) {
        usleep(1000);
        ui_handle_events();
    }

    return 0;
}

void emu_cycles(int cpu_cycles) {
	//TODOOOO
}