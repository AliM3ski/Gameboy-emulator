// audio.c - SDL audio backend for Gameboy emulator
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "apu.h"

#define SAMPLE_RATE   48000    // Output sample rate
#define BUFFER_SIZE   8192     // Circular buffer size

static SDL_AudioDeviceID audio_dev = 0;

// Circular buffer
static float audio_buffer[BUFFER_SIZE];
static int write_pos = 0;
static int read_pos = 0;

// Smooth filter to reduce artifacts
static float prev_sample = 0.0f;

// Enqueue a single sample into circular buffer
void audio_enqueue(float sample) {
    audio_buffer[write_pos] = sample;
    write_pos = (write_pos + 1) % BUFFER_SIZE;
}

// SDL audio callback
static void audio_callback(void *userdata, Uint8 *stream, int len) {
    (void)userdata;
    int samples = len / sizeof(float);
    float *out = (float *)stream;

    for (int i = 0; i < samples; i++) {
        if (read_pos == write_pos) {
            out[i] = 0.0f; // buffer underrun → output silence
        } else {
            // optional low-pass filter for smoother output
            float s = audio_buffer[read_pos];
            s = prev_sample * 0.8f + s * 0.2f;
            prev_sample = s;

            out[i] = s;
            read_pos = (read_pos + 1) % BUFFER_SIZE;
        }
    }
}

// Initialize SDL audio
bool audio_init(void) {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = SAMPLE_RATE;
    want.format = AUDIO_F32SYS;   // float samples [-1.0, 1.0]
    want.channels = 1;
    want.samples = 2048;          // buffer for callback
    want.callback = audio_callback;

    audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!audio_dev) {
        fprintf(stderr, "SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_PauseAudioDevice(audio_dev, 0); // start playback
    printf("SDL Audio initialized: %d Hz, %d channels\n", have.freq, have.channels);
    return true;
}

// Close audio
void audio_close(void) {
    if (audio_dev) SDL_CloseAudioDevice(audio_dev);
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

// Called by emulator per CPU batch or per frame
void audio_submit(float sample) {
    // clamp sample to [-1.0, 1.0] to avoid clipping
    if (sample > 1.0f) sample = 1.0f;
    if (sample < -1.0f) sample = -1.0f;
    audio_enqueue(sample);
}
