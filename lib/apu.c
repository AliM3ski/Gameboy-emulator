// apu.c - Fixed Gameboy APU Implementation
#include <SDL2/SDL.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "apu.h"

#define SAMPLE_RATE 48000
#define BUFFER_SIZE 8192
#define GB_CPU_HZ 4194304

// --- Channel structs ---
typedef struct {
    float phase;
    float freq_hz;
    float duty;
    float volume;
    int enabled;
    // Envelope
    uint8_t env_volume;
    uint8_t env_initial_volume;
    uint8_t env_direction;
    uint8_t env_period;
    int env_timer;
    // Length counter
    int length_enabled;
    int length_counter;
    // Sweep (CH1 only)
    uint8_t sweep_period;
    uint8_t sweep_direction;
    uint8_t sweep_shift;
    int sweep_timer;
    uint16_t sweep_frequency;
} Square;

typedef struct {
    float phase;
    float freq_hz;
    float volume;
    int enabled;
    uint8_t waveform[32];
    int length_enabled;
    int length_counter;
} Wave;

typedef struct {
    float phase;
    float freq_hz;
    float volume;
    int enabled;
    uint32_t lfsr;
    int width_mode;
    // Envelope
    uint8_t env_volume;
    uint8_t env_initial_volume;
    uint8_t env_direction;
    uint8_t env_period;
    int env_timer;
    // Length counter
    int length_enabled;
    int length_counter;
} Noise;

static Square ch1, ch2;
static Wave ch3;
static Noise ch4;

// Master control
static int master_enabled = 1;
static float master_volume_left = 1.0f;
static float master_volume_right = 1.0f;

// Debugging flags
static int dbg_mute_ch1 = 0;
static int dbg_mute_ch2 = 0;
static int dbg_mute_ch3 = 0;
static int dbg_mute_ch4 = 0;

// --- Registers 0xFF10–0xFF3F ---
static uint8_t apu_regs[0x30];

// --- Audio buffer ---
static float audio_buffer[BUFFER_SIZE];
static volatile int write_pos = 0;
static volatile int read_pos = 0;
static SDL_AudioDeviceID audio_dev = 0;
static SDL_mutex *audio_mutex = NULL;

// Frame sequencer
static int frame_sequencer = 0;
static int frame_sequencer_counter = 0;

// Duty cycle patterns
static const float duty_patterns[4][8] = {
    {0, 0, 0, 0, 0, 0, 0, 1},  // 12.5%
    {1, 0, 0, 0, 0, 0, 0, 1},  // 25%
    {1, 0, 0, 0, 0, 1, 1, 1},  // 50%
    {0, 1, 1, 1, 1, 1, 1, 0}   // 75%
};

// --- Helpers ---
static int buffer_space_available(void) {
    int space = read_pos - write_pos - 1;
    if (space < 0) space += BUFFER_SIZE;
    return space;
}

static void enqueue_sample(float s) {
    if (buffer_space_available() > 0) {
        if (s > 1.0f) s = 1.0f;
        if (s < -1.0f) s = -1.0f;
        audio_buffer[write_pos] = s;
        write_pos = (write_pos + 1) % BUFFER_SIZE;
    }
}

static void audio_callback(void *userdata, Uint8 *stream, int len) {
    (void)userdata;
    float *out = (float*)stream;
    int samples = len / sizeof(float);
    
    SDL_LockMutex(audio_mutex);
    for (int i = 0; i < samples; i++) {
        if (read_pos != write_pos) {
            out[i] = audio_buffer[read_pos];
            read_pos = (read_pos + 1) % BUFFER_SIZE;
        } else {
            out[i] = 0.0f;
        }
    }
    SDL_UnlockMutex(audio_mutex);
}

// --- Envelope processing ---
static void update_envelope(Square *ch) {
    if (ch->env_period == 0) return;
    
    if (--ch->env_timer <= 0) {
        ch->env_timer = ch->env_period * (GB_CPU_HZ / 64);
        
        if (ch->env_direction && ch->env_volume < 15) {
            ch->env_volume++;
        } else if (!ch->env_direction && ch->env_volume > 0) {
            ch->env_volume--;
        }
        
        ch->volume = ch->env_volume / 15.0f;
    }
}

static void update_envelope_noise(Noise *ch) {
    if (ch->env_period == 0) return;
    
    if (--ch->env_timer <= 0) {
        ch->env_timer = ch->env_period * (GB_CPU_HZ / 64);
        
        if (ch->env_direction && ch->env_volume < 15) {
            ch->env_volume++;
        } else if (!ch->env_direction && ch->env_volume > 0) {
            ch->env_volume--;
        }
        
        ch->volume = ch->env_volume / 15.0f;
    }
}

// --- Sweep processing (CH1 only) ---
static void update_sweep(Square *ch) {
    if (ch->sweep_period == 0) return;
    
    if (--ch->sweep_timer <= 0) {
        ch->sweep_timer = ch->sweep_period * (GB_CPU_HZ / 128);
        
        uint16_t new_freq = ch->sweep_frequency;
        uint16_t freq_delta = new_freq >> ch->sweep_shift;
        
        if (ch->sweep_direction) {
            // Decrease frequency (increase pitch)
            if (new_freq >= freq_delta) {
                new_freq -= freq_delta;
            }
        } else {
            // Increase frequency (decrease pitch)
            new_freq += freq_delta;
            if (new_freq > 2047) {
                ch->enabled = 0;
                return;
            }
        }
        
        ch->sweep_frequency = new_freq;
        ch->freq_hz = 131072.0f / (2048 - new_freq);
    }
}

// --- Length counter processing ---
static void update_length(Square *ch) {
    if (ch->length_enabled && ch->length_counter > 0) {
        ch->length_counter--;
        if (ch->length_counter == 0) {
            ch->enabled = 0;
        }
    }
}

static void update_length_wave(Wave *ch) {
    if (ch->length_enabled && ch->length_counter > 0) {
        ch->length_counter--;
        if (ch->length_counter == 0) {
            ch->enabled = 0;
        }
    }
}

static void update_length_noise(Noise *ch) {
    if (ch->length_enabled && ch->length_counter > 0) {
        ch->length_counter--;
        if (ch->length_counter == 0) {
            ch->enabled = 0;
        }
    }
}

// --- Channel samples ---
static float square_sample(Square *ch) {
    if (!ch->enabled || !master_enabled) return 0.0f;
    
    int duty_index = (int)(ch->phase * 8.0f) & 7;
    float s = duty_patterns[(int)(ch->duty * 3.99f)][duty_index] ? ch->volume : 0.0f;
    
    ch->phase += ch->freq_hz / SAMPLE_RATE;
    while (ch->phase >= 1.0f) ch->phase -= 1.0f;
    
    return s;
}

static float wave_sample(Wave *ch) {
    if (!ch->enabled || !master_enabled) return 0.0f;
    
    int idx = (int)(ch->phase * 32.0f) & 31;
    float s = (ch->waveform[idx] / 15.0f - 0.5f) * ch->volume * 2.0f;
    
    ch->phase += ch->freq_hz / SAMPLE_RATE;
    while (ch->phase >= 1.0f) ch->phase -= 1.0f;
    
    return s;
}

static float noise_sample(Noise *ch) {
    if (!ch->enabled || !master_enabled) return 0.0f;
    
    static float last = 0.0f;
    ch->phase += ch->freq_hz / SAMPLE_RATE;
    
    if (ch->phase >= 1.0f) {
        ch->phase -= 1.0f;
        
        // LFSR feedback
        int bit = ((ch->lfsr >> 0) ^ (ch->lfsr >> 1)) & 1;
        ch->lfsr = (ch->lfsr >> 1) | (bit << 14);
        
        if (ch->width_mode) {
            ch->lfsr &= ~(1 << 6);
            ch->lfsr |= bit << 6;
        }
        
        last = (ch->lfsr & 1) ? 0.0f : ch->volume;
    }
    
    return last;
}

void apu_init(void) {
    memset(apu_regs, 0, sizeof(apu_regs));
    memset(&ch1, 0, sizeof(ch1));
    memset(&ch2, 0, sizeof(ch2));
    memset(&ch3, 0, sizeof(ch3));
    memset(&ch4, 0, sizeof(ch4));
    
    ch1.volume = 0.0f;
    ch2.volume = 0.0f;
    ch3.volume = 0.5f;
    ch4.volume = 0.0f;
    ch4.lfsr = 0x7FFF;
    
    // Initialize default wave pattern
    for (int i = 0; i < 16; i++) {
        ch3.waveform[i * 2] = (i < 8) ? 0 : 15;
        ch3.waveform[i * 2 + 1] = (i < 8) ? 0 : 15;
    }
    
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return;
    }
    
    audio_mutex = SDL_CreateMutex();
    
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = SAMPLE_RATE;
    want.format = AUDIO_F32SYS;
    want.channels = 1;
    want.samples = 512;
    want.callback = audio_callback;
    
    audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!audio_dev) {
        fprintf(stderr, "SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
        return;
    }
    
    // Pre-fill buffer with silence
    for (int i = 0; i < BUFFER_SIZE; i++) {
        audio_buffer[i] = 0.0f;
    }
    
    SDL_PauseAudioDevice(audio_dev, 0);
    printf("APU initialized\n");
}

void apu_quit(void) {
    if (audio_dev) SDL_CloseAudioDevice(audio_dev);
    if (audio_mutex) SDL_DestroyMutex(audio_mutex);
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

// --- Step ---
void apu_step(int cycles) {
    static double acc = 0;
    acc += cycles;
    
    // Frame sequencer (512 Hz)
    frame_sequencer_counter += cycles;
    if (frame_sequencer_counter >= GB_CPU_HZ / 512) {
        frame_sequencer_counter -= GB_CPU_HZ / 512;
        frame_sequencer = (frame_sequencer + 1) & 7;
        
        // Clock length counters at 256 Hz (every other frame)
        if ((frame_sequencer & 1) == 0) {
            update_length(&ch1);
            update_length(&ch2);
            update_length_wave(&ch3);
            update_length_noise(&ch4);
        }
        
        // Clock sweep at 128 Hz (frames 2 and 6)
        if (frame_sequencer == 2 || frame_sequencer == 6) {
            update_sweep(&ch1);
        }
        
        // Clock envelopes at 64 Hz (frame 7)
        if (frame_sequencer == 7) {
            update_envelope(&ch1);
            update_envelope(&ch2);
            update_envelope_noise(&ch4);
        }
    }
    
    double cycles_per_sample = (double)GB_CPU_HZ / SAMPLE_RATE;
    while (acc >= cycles_per_sample) {
        acc -= cycles_per_sample;
        
        float s = 0.0f;
        if (!dbg_mute_ch1) s += square_sample(&ch1);
        if (!dbg_mute_ch2) s += square_sample(&ch2);
        if (!dbg_mute_ch3) s += wave_sample(&ch3);
        if (!dbg_mute_ch4) s += noise_sample(&ch4);
        
        // Mix and apply master volume
        s *= 0.25f;
        
        SDL_LockMutex(audio_mutex);
        enqueue_sample(s);
        SDL_UnlockMutex(audio_mutex);
    }
}

// --- Read / Write ---
uint8_t apu_read(uint16_t addr) {
    if (addr >= 0xFF10 && addr <= 0xFF3F) {
        // Some registers are write-only or have unused bits
        if (addr == 0xFF10) return apu_regs[addr - 0xFF10] | 0x80;
        if (addr == 0xFF11 || addr == 0xFF16) return apu_regs[addr - 0xFF10] | 0x3F;
        if (addr == 0xFF13 || addr == 0xFF18 || addr == 0xFF1B || addr == 0xFF1D || addr == 0xFF20) return 0xFF;
        if (addr == 0xFF14 || addr == 0xFF19 || addr == 0xFF1E || addr == 0xFF23) return apu_regs[addr - 0xFF10] | 0xBF;
        if (addr == 0xFF15) return 0xFF;
        if (addr == 0xFF1A) return apu_regs[addr - 0xFF10] | 0x7F;
        if (addr == 0xFF1C) return apu_regs[addr - 0xFF10] | 0x9F;
        if (addr == 0xFF1F) return 0xFF;
        if (addr == 0xFF26) return (master_enabled ? 0x80 : 0) | (ch1.enabled ? 1 : 0) | (ch2.enabled ? 2 : 0) | (ch3.enabled ? 4 : 0) | (ch4.enabled ? 8 : 0) | 0x70;
        if (addr >= 0xFF27 && addr <= 0xFF2F) return 0xFF;
        return apu_regs[addr - 0xFF10];
    }
    return 0xFF;
}

void apu_write(uint16_t addr, uint8_t val) {
    if (addr < 0xFF10 || addr > 0xFF3F) return;
    
    // Check if APU is enabled (except for wave RAM and length counters)
    if (!master_enabled && addr != 0xFF26 && !(addr >= 0xFF30 && addr <= 0xFF3F)) {
        // Only length counters can be written while disabled
        if (addr != 0xFF11 && addr != 0xFF16 && addr != 0xFF1B && addr != 0xFF20) {
            return;
        }
    }
    
    apu_regs[addr - 0xFF10] = val;
    
    // --- Channel 1 (Square with sweep) ---
    if (addr == 0xFF10) {
        // Sweep
        ch1.sweep_period = (val >> 4) & 7;
        ch1.sweep_direction = (val >> 3) & 1;
        ch1.sweep_shift = val & 7;
    }
    if (addr == 0xFF11) {
        // Duty & Length
        ch1.duty = ((val >> 6) & 3) / 3.0f;
        ch1.length_counter = 64 - (val & 0x3F);
    }
    if (addr == 0xFF12) {
        // Envelope
        ch1.env_initial_volume = (val >> 4) & 0xF;
        ch1.env_direction = (val >> 3) & 1;
        ch1.env_period = val & 7;
        if ((val & 0xF8) == 0) ch1.enabled = 0;
    }
    if (addr == 0xFF13 || addr == 0xFF14) {
        uint16_t freq = (uint16_t)apu_regs[0x03] | ((uint16_t)(apu_regs[0x04] & 7) << 8);
        ch1.freq_hz = 131072.0f / (2048 - freq);
        ch1.sweep_frequency = freq;
        
        if (addr == 0xFF14) {
            ch1.length_enabled = (val >> 6) & 1;
            if (val & 0x80) {
                // Trigger
                ch1.enabled = 1;
                ch1.phase = 0;
                ch1.env_volume = ch1.env_initial_volume;
                ch1.volume = ch1.env_volume / 15.0f;
                ch1.env_timer = ch1.env_period * (GB_CPU_HZ / 64);
                ch1.sweep_timer = ch1.sweep_period * (GB_CPU_HZ / 128);
                if (ch1.length_counter == 0) ch1.length_counter = 64;
                if ((apu_regs[0x02] & 0xF8) == 0) ch1.enabled = 0;
            }
        }
    }
    
    // --- Channel 2 (Square) ---
    if (addr == 0xFF16) {
        // Duty & Length
        ch2.duty = ((val >> 6) & 3) / 3.0f;
        ch2.length_counter = 64 - (val & 0x3F);
    }
    if (addr == 0xFF17) {
        // Envelope
        ch2.env_initial_volume = (val >> 4) & 0xF;
        ch2.env_direction = (val >> 3) & 1;
        ch2.env_period = val & 7;
        if ((val & 0xF8) == 0) ch2.enabled = 0;
    }
    if (addr == 0xFF18 || addr == 0xFF19) {
        uint16_t freq = (uint16_t)apu_regs[0x08] | ((uint16_t)(apu_regs[0x09] & 7) << 8);
        ch2.freq_hz = 131072.0f / (2048 - freq);
        
        if (addr == 0xFF19) {
            ch2.length_enabled = (val >> 6) & 1;
            if (val & 0x80) {
                // Trigger
                ch2.enabled = 1;
                ch2.phase = 0;
                ch2.env_volume = ch2.env_initial_volume;
                ch2.volume = ch2.env_volume / 15.0f;
                ch2.env_timer = ch2.env_period * (GB_CPU_HZ / 64);
                if (ch2.length_counter == 0) ch2.length_counter = 64;
                if ((apu_regs[0x07] & 0xF8) == 0) ch2.enabled = 0;
            }
        }
    }
    
    // --- Channel 3 (Wave) ---
    if (addr == 0xFF1A) {
        // DAC enable
        if (!(val & 0x80)) ch3.enabled = 0;
    }
    if (addr == 0xFF1B) {
        // Length
        ch3.length_counter = 256 - val;
    }
    if (addr == 0xFF1C) {
        // Volume
        int vol_shift = (val >> 5) & 3;
        if (vol_shift == 0) ch3.volume = 0.0f;
        else if (vol_shift == 1) ch3.volume = 1.0f;
        else if (vol_shift == 2) ch3.volume = 0.5f;
        else ch3.volume = 0.25f;
    }
    if (addr == 0xFF1D || addr == 0xFF1E) {
        uint16_t freq = (uint16_t)apu_regs[0x0D] | ((uint16_t)(apu_regs[0x0E] & 7) << 8);
        ch3.freq_hz = 65536.0f / (2048 - freq);
        
        if (addr == 0xFF1E) {
            ch3.length_enabled = (val >> 6) & 1;
            if (val & 0x80) {
                // Trigger
                if (apu_regs[0x0A] & 0x80) {
                    ch3.enabled = 1;
                    ch3.phase = 0;
                    if (ch3.length_counter == 0) ch3.length_counter = 256;
                }
            }
        }
    }
    if (addr >= 0xFF30 && addr <= 0xFF3F) {
        // Wave pattern RAM
        int i = (addr - 0xFF30) * 2;
        ch3.waveform[i] = (val >> 4) & 0xF;
        ch3.waveform[i + 1] = val & 0xF;
    }
    
    // --- Channel 4 (Noise) ---
    if (addr == 0xFF20) {
        // Length
        ch4.length_counter = 64 - (val & 0x3F);
    }
    if (addr == 0xFF21) {
        // Envelope
        ch4.env_initial_volume = (val >> 4) & 0xF;
        ch4.env_direction = (val >> 3) & 1;
        ch4.env_period = val & 7;
        if ((val & 0xF8) == 0) ch4.enabled = 0;
    }
    if (addr == 0xFF22) {
        // Polynomial counter
        int shift = (val >> 4) & 0xF;
        int divisor_code = val & 7;
        ch4.width_mode = (val >> 3) & 1;
        
        float divisor = divisor_code ? (divisor_code * 16.0f) : 8.0f;
        ch4.freq_hz = 524288.0f / divisor / (1 << (shift + 1));
    }
    if (addr == 0xFF23) {
        ch4.length_enabled = (val >> 6) & 1;
        if (val & 0x80) {
            // Trigger
            ch4.enabled = 1;
            ch4.phase = 0;
            ch4.lfsr = 0x7FFF;
            ch4.env_volume = ch4.env_initial_volume;
            ch4.volume = ch4.env_volume / 15.0f;
            ch4.env_timer = ch4.env_period * (GB_CPU_HZ / 64);
            if (ch4.length_counter == 0) ch4.length_counter = 64;
            if ((apu_regs[0x11] & 0xF8) == 0) ch4.enabled = 0;
        }
    }
    
    // --- Master control ---
    if (addr == 0xFF24) {
        // Master volume
        master_volume_left = ((val >> 4) & 7) / 7.0f;
        master_volume_right = (val & 7) / 7.0f;
    }
    if (addr == 0xFF26) {
        // Master enable
        master_enabled = (val >> 7) & 1;
        if (!master_enabled) {
            // Disable all channels
            ch1.enabled = 0;
            ch2.enabled = 0;
            ch3.enabled = 0;
            ch4.enabled = 0;
            // Clear all registers except wave RAM
            for (int i = 0; i < 0x30; i++) {
                if (i < 0x20 || i >= 0x30) {
                    apu_regs[i] = 0;
                }
            }
        }
    }
}

// Debug control functions
void apu_mute_channel(int ch, int mute) {
    switch (ch) {
        case 1: dbg_mute_ch1 = !!mute; break;
        case 2: dbg_mute_ch2 = !!mute; break;
        case 3: dbg_mute_ch3 = !!mute; break;
        case 4: dbg_mute_ch4 = !!mute; break;
    }
}

void apu_log_underruns(int enable) {
    // Not used in this version
    (void)enable;
}