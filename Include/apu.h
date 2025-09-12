#pragma once
#include <common.h>

// Initialize APU + SDL audio
void apu_init();

// Cleanup on shutdown
void apu_quit();

// Called each CPU cycle
void apu_step(int cycles);

// I/O mapping
u8 apu_read(u16 address);
void apu_write(u16 address, u8 value);

