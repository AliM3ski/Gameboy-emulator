#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool enabled;
    int duty;          // 0-3 (12.5%,25%,50%,75%)
    int freq;          // 11-bit frequency
    int volume;        // 0-15 current volume
    int init_volume;   // envelope start (NRx2 high nibble)
    int envelope_period; // envelope period (0-7)
    int envelope_timer;  // runtime timer for envelope
    int envelope_dir;   // +1 or -1
    int length;        // sound length (counts down)
    bool length_enable;
    double phase;
    // Sweep (channel 1 only)
    int sweep_period;
    int sweep_timer;
    int sweep_shift;
    int sweep_neg;    // 0 = add, 1 = subtract
    int shadow_freq;  // used for sweep calculations
    bool sweep_reload; // on trigger
} SquareChannel;

typedef struct {
    bool enabled;
    uint8_t wave[32];  // 4-bit samples (0..15) stored as bytes
    int volume_shift;  // 0=100%, 1=100%? (NRx4: 00=mute, 01=100%, 10=50%, 11=25%) we'll implement 0..3 mapping
    int freq;
    int length;
    bool length_enable;
    double phase;
    bool dac_power; // NR30
} WaveChannel;

typedef struct {
    bool enabled;
    int volume;        // current volume 0-15
    int init_volume;   // NR42 high nibble
    int envelope_period;
    int envelope_timer;
    int envelope_dir;
    int length;
    bool length_enable;
    uint16_t lfsr;
    double phase;
    // polynomial / clock
    int r; // shift clock frequency select (0..7)
    int s; // dividing ratio of frequencies (0..7 actually 0..0xF in reg; we use simplified)
    bool width_mode; // 7-bit LFSR when true
} NoiseChannel;

typedef struct {
    SquareChannel ch1;
    SquareChannel ch2;
    WaveChannel ch3;
    NoiseChannel ch4;
    // register mirror for reads and some control bits
    uint8_t regs[0x30]; // 0xFF10..0xFF3F
} APUContext;

void apu_init();
void apu_write(uint16_t addr, uint8_t val);
uint8_t apu_read(uint16_t addr);
void apu_step(int cycles);

