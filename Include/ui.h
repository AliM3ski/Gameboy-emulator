#pragma once
#include <common.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// Game Boy screen resolution
static const int SCREEN_WIDTH = 160;
static const int SCREEN_HEIGHT = 144;

// --- Public UI API ---
// Initialization and cleanup
void ui_init(void);
void ui_cleanup(void);

// Frame update (rendering)
void ui_update(void);

// Event handling
void ui_handle_events(void);

// Timing helpers
void delay(u32 ms);
u32 get_ticks(void);

// Fullscreen and scaling toggles
void toggle_fullscreen(void);
void toggle_integer_scaling(void);
void toggle_aspect_ratio(void);

// Tile viewer (debug window helpers)
void display_tile(SDL_Surface *surface, u16 startLocation, u16 tileNum, int x, int y);

// Keyboard input handler
void ui_on_key(bool down, u32 key_code);