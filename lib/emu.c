#include <stdio.h>
#include <emu.h>
#include <cart.h>
#include <cpu.h>
#include <SDL2/SDL.h> // for graphics library
#include <SDL2/SDL_ttf.h> // for graphics library

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

void delay(u32 ms) {
	SDL_Delay(ms);
}

int emu_run(int argc, char **argv) {
	// check to see if they passed in rom file
	if (argc < 2) {
		printf("ROM file not passed. Please pass in a ROM file\n");
		return -1;
	}
	// return error if loading cart fails
	if (!cart_load(argv[1])) {
		printf("Failed to load ROM file: %s\n", argv[1]);
		return -2;
	}

	printf("Cart loaded..\n");

	SDL_Init(SDL_INIT_VIDEO);
	printf("SDL Initalize\n");
	TTF_Init();
	printf("TTF initalize\n");

	cpu_init();

	context.running = true;
	context.paused = false;
	context.ticks = 0;

	// game loop
	while(context.running) {
		if (context.paused) {
			delay(10);
			continue;
		}

		if (!cpu_step()) {
			printf("CPU Stopped\n");
			return -3
		}
		context.ticks++;
	}
	return 0;
}