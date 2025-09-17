#include <ui.h>
#include <emu.h>
#include <bus.h>
#include <ppu.h>
#include <gamepad.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// Global SDL objects for main emulator window
SDL_Window *sdlWindow;
SDL_Renderer *sdlRenderer;
SDL_Texture *sdlTexture;
SDL_Surface *screen;

// Global SDL objects for debug window (tile viewer)
SDL_Window *sdlDebugWindow = NULL;
SDL_Renderer *sdlDebugRenderer = NULL;
SDL_Texture *sdlDebugTexture = NULL;
SDL_Surface *debugScreen = NULL;

// Window and scaling properties
static int window_width = SCREEN_WIDTH * 4;   // Initial window size
static int window_height = SCREEN_HEIGHT * 4;
static float scale_x = 4.0f;
static float scale_y = 4.0f;
static SDL_Rect viewport;
static bool maintain_aspect_ratio = true;
static bool use_integer_scaling = true;
static bool fullscreen = false;
static bool vsync_enabled = true;

// UI color scheme
static const u32 UI_BG_COLOR = 0xFF1A1A1A;  // Dark gray background
static const u32 UI_BORDER_COLOR = 0xFF333333;  // Slightly lighter border

void calculate_viewport() {
    int win_w, win_h;
    SDL_GetWindowSize(sdlWindow, &win_w, &win_h);
    
    if (maintain_aspect_ratio) {
        float gb_aspect = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
        float win_aspect = (float)win_w / (float)win_h;
        
        if (use_integer_scaling) {
            // Calculate the best integer scale that fits
            int scale_int_x = win_w / SCREEN_WIDTH;
            int scale_int_y = win_h / SCREEN_HEIGHT;
            int best_scale = (scale_int_x < scale_int_y) ? scale_int_x : scale_int_y;
            
            if (best_scale < 1) best_scale = 1;
            
            viewport.w = SCREEN_WIDTH * best_scale;
            viewport.h = SCREEN_HEIGHT * best_scale;
        } else {
            // Smooth scaling with aspect ratio preservation
            if (win_aspect > gb_aspect) {
                // Window is wider than GB aspect
                viewport.h = win_h;
                viewport.w = (int)(win_h * gb_aspect);
            } else {
                // Window is taller than GB aspect
                viewport.w = win_w;
                viewport.h = (int)(win_w / gb_aspect);
            }
        }
        
        // Center the viewport
        viewport.x = (win_w - viewport.w) / 2;
        viewport.y = (win_h - viewport.h) / 2;
    } else {
        // Stretch to fill window
        viewport.x = 0;
        viewport.y = 0;
        viewport.w = win_w;
        viewport.h = win_h;
    }
    
    scale_x = (float)viewport.w / SCREEN_WIDTH;
    scale_y = (float)viewport.h / SCREEN_HEIGHT;
}

void ui_init() {
    // Initialize SDL with video and audio subsystems
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return;
    }
    printf("SDL INIT\n");
    
    if (TTF_Init() < 0) {
        printf("TTF initialization failed: %s\n", TTF_GetError());
    }
    printf("TTF INIT\n");

    // Set rendering hints for better quality
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");  // Nearest neighbor for pixel art
    
    // Create main emulator window with resizable flag
    sdlWindow = SDL_CreateWindow(
        "Game Boy Emulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        window_width,
        window_height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );
    
    if (!sdlWindow) {
        printf("Window creation failed: %s\n", SDL_GetError());
        return;
    }
    
    // Set minimum window size to prevent too small windows
    SDL_SetWindowMinimumSize(sdlWindow, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    // Create renderer with VSync if enabled
    Uint32 renderer_flags = SDL_RENDERER_ACCELERATED;
    if (vsync_enabled) {
        renderer_flags |= SDL_RENDERER_PRESENTVSYNC;
    }
    
    sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, renderer_flags);
    if (!sdlRenderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        return;
    }
    
    // Create a software surface for drawing emulator pixels
    screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
                                  0x00FF0000, // R mask
                                  0x0000FF00, // G mask
                                  0x000000FF, // B mask
                                  0xFF000000);// A mask
    
    if (!screen) {
        printf("Surface creation failed: %s\n", SDL_GetError());
        return;
    }

    // Create a texture for uploading pixels to the GPU
    sdlTexture = SDL_CreateTexture(sdlRenderer,
                                   SDL_PIXELFORMAT_ARGB8888,
                                   SDL_TEXTUREACCESS_STREAMING,
                                   SCREEN_WIDTH, SCREEN_HEIGHT);
    
    if (!sdlTexture) {
        printf("Texture creation failed: %s\n", SDL_GetError());
        return;
    }
    
    // Calculate initial viewport
    calculate_viewport();
    
    printf("UI initialization complete. Screen size: %dx%d\n", SCREEN_WIDTH, SCREEN_HEIGHT);
}

void delay(u32 ms) {
    SDL_Delay(ms);
}

u32 get_ticks() {
    return SDL_GetTicks();
}

// Enhanced tile colors with better contrast
static unsigned long tile_colors[4] = {
    0xFFE0F8D0,  // Lightest green (GB classic)
    0xFF88C070,  // Light green
    0xFF346856,  // Dark green
    0xFF081820   // Darkest green
};

void display_tile(SDL_Surface *surface, u16 startLocation, u16 tileNum, int x, int y) {
    if (!surface) return;
    
    SDL_Rect rc;
    int tile_scale = 2;  // Fixed scale for tile viewer

    for (int tileY = 0; tileY < 16; tileY += 2) {
        u8 b1 = bus_read(startLocation + (tileNum * 16) + tileY);
        u8 b2 = bus_read(startLocation + (tileNum * 16) + tileY + 1);

        for (int bit = 7; bit >= 0; bit--) {
            u8 hi = !!(b1 & (1 << bit)) << 1;
            u8 lo = !!(b2 & (1 << bit));
            u8 color = hi | lo;

            rc.x = x + ((7 - bit) * tile_scale);
            rc.y = y + (tileY / 2 * tile_scale);
            rc.w = tile_scale;
            rc.h = tile_scale;

            SDL_FillRect(surface, &rc, tile_colors[color]);
        }
    }
}

void ui_update() {
    // CRITICAL: Add safety checks for PPU context
    ppu_context *ppu_ctx = ppu_get_context();
    if (!ppu_ctx) {
        printf("ERROR: PPU context is NULL!\n");
        return;
    }
    
    if (!ppu_ctx->video_buffer) {
        printf("ERROR: PPU video buffer is NULL!\n");
        return;
    }
    
    // Verify screen dimensions are valid
    if (SCREEN_WIDTH <= 0 || SCREEN_HEIGHT <= 0) {
        printf("ERROR: Invalid screen dimensions: %dx%d\n", SCREEN_WIDTH, SCREEN_HEIGHT);
        return;
    }
    
    // Safety check for SDL objects
    if (!screen || !sdlTexture || !sdlRenderer) {
        printf("ERROR: SDL objects not properly initialized\n");
        return;
    }
    
    u32 *video_buffer = ppu_ctx->video_buffer;
    
    // Lock the surface for direct pixel access (more efficient)
    if (SDL_LockSurface(screen) < 0) {
        printf("ERROR: Could not lock surface: %s\n", SDL_GetError());
        return;
    }
    
    u32 *pixels = (u32*)screen->pixels;
    
    // Copy framebuffer directly to surface with bounds checking
    int total_pixels = SCREEN_WIDTH * SCREEN_HEIGHT;
    for (int i = 0; i < total_pixels; i++) {
        pixels[i] = video_buffer[i];
    }
    
    SDL_UnlockSurface(screen);
    
    // Update texture with new pixel data
    if (SDL_UpdateTexture(sdlTexture, NULL, screen->pixels, screen->pitch) < 0) {
        printf("ERROR: Could not update texture: %s\n", SDL_GetError());
        return;
    }
    
    // Clear renderer with background color
    SDL_SetRenderDrawColor(sdlRenderer, 
        (UI_BG_COLOR >> 16) & 0xFF,
        (UI_BG_COLOR >> 8) & 0xFF,
        UI_BG_COLOR & 0xFF,
        0xFF);
    SDL_RenderClear(sdlRenderer);
    
    // Draw the Game Boy screen with proper viewport
    SDL_RenderSetViewport(sdlRenderer, &viewport);
    SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
    SDL_RenderSetViewport(sdlRenderer, NULL);
    
    // Optional: Draw a border around the viewport
    if (maintain_aspect_ratio) {
        SDL_SetRenderDrawColor(sdlRenderer,
            (UI_BORDER_COLOR >> 16) & 0xFF,
            (UI_BORDER_COLOR >> 8) & 0xFF,
            UI_BORDER_COLOR & 0xFF,
            0xFF);
        SDL_Rect border = {viewport.x - 1, viewport.y - 1, viewport.w + 2, viewport.h + 2};
        SDL_RenderDrawRect(sdlRenderer, &border);
    }
    
    // Present the frame
    SDL_RenderPresent(sdlRenderer);
}

void toggle_fullscreen() {
    fullscreen = !fullscreen;
    SDL_SetWindowFullscreen(sdlWindow, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    calculate_viewport();
}

void toggle_integer_scaling() {
    use_integer_scaling = !use_integer_scaling;
    calculate_viewport();
}

void toggle_aspect_ratio() {
    maintain_aspect_ratio = !maintain_aspect_ratio;
    calculate_viewport();
}

void ui_on_key(bool down, u32 key_code) {
    // Safety check for gamepad context
    gamepad_state *gamepad = gamepad_get_state();
    if (!gamepad) {
        printf("WARNING: Gamepad state is NULL\n");
        return;
    }
    
    // Game controls
    switch (key_code) {
        case SDLK_z:      gamepad->b      = down; break;
        case SDLK_x:      gamepad->a      = down; break;
        case SDLK_RETURN: gamepad->start  = down; break;
        case SDLK_TAB:    gamepad->select = down; break;
        case SDLK_UP:     gamepad->up     = down; break;
        case SDLK_DOWN:   gamepad->down   = down; break;
        case SDLK_LEFT:   gamepad->left   = down; break;
        case SDLK_RIGHT:  gamepad->right  = down; break;
    }
    
    // UI controls (only on key down)
    if (down) {
        switch (key_code) {
            case SDLK_F11:
                toggle_fullscreen();
                break;
            case SDLK_i:
                if (SDL_GetModState() & KMOD_CTRL) {
                    toggle_integer_scaling();
                    printf("Integer scaling: %s\n", use_integer_scaling ? "ON" : "OFF");
                }
                break;
            case SDLK_a:
                if (SDL_GetModState() & KMOD_CTRL) {
                    toggle_aspect_ratio();
                    printf("Maintain aspect ratio: %s\n", maintain_aspect_ratio ? "ON" : "OFF");
                }
                break;
            case SDLK_ESCAPE:
                if (fullscreen) {
                    toggle_fullscreen();
                }
                break;
            // Quick save states (if you implement them)
            case SDLK_F5:
                // save_state();
                printf("Save state (F5) - Not yet implemented\n");
                break;
            case SDLK_F7:
                // load_state();
                printf("Load state (F7) - Not yet implemented\n");
                break;
        }
    }
}

void ui_handle_events() {
    SDL_Event e;
    while (SDL_PollEvent(&e) > 0) {
        switch (e.type) {
            case SDL_KEYDOWN:
                ui_on_key(true, e.key.keysym.sym);
                break;
                
            case SDL_KEYUP:
                ui_on_key(false, e.key.keysym.sym);
                break;
                
            case SDL_WINDOWEVENT:
                switch (e.window.event) {
                    case SDL_WINDOWEVENT_CLOSE:
                        if (emu_get_context()) {
                            emu_get_context()->die = true;
                        }
                        break;
                        
                    case SDL_WINDOWEVENT_RESIZED:
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                        calculate_viewport();
                        break;
                        
                    case SDL_WINDOWEVENT_MAXIMIZED:
                    case SDL_WINDOWEVENT_RESTORED:
                        calculate_viewport();
                        break;
                }
                break;
                
            case SDL_QUIT:
                if (emu_get_context()) {
                    emu_get_context()->die = true;
                }
                break;
                
            // Mouse wheel for quick scaling adjustment
            case SDL_MOUSEWHEEL:
                if (SDL_GetModState() & KMOD_CTRL) {
                    int win_w, win_h;
                    SDL_GetWindowSize(sdlWindow, &win_w, &win_h);
                    
                    if (e.wheel.y > 0) {
                        // Zoom in
                        win_w = (int)(win_w * 1.1f);
                        win_h = (int)(win_h * 1.1f);
                    } else if (e.wheel.y < 0) {
                        // Zoom out
                        win_w = (int)(win_w * 0.9f);
                        win_h = (int)(win_h * 0.9f);
                    }
                    
                    // Clamp to reasonable sizes
                    if (win_w < SCREEN_WIDTH) win_w = SCREEN_WIDTH;
                    if (win_h < SCREEN_HEIGHT) win_h = SCREEN_HEIGHT;
                    if (win_w > 1920) win_w = 1920;
                    if (win_h > 1080) win_h = 1080;
                    
                    SDL_SetWindowSize(sdlWindow, win_w, win_h);
                    SDL_SetWindowPosition(sdlWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
                    calculate_viewport();
                }
                break;
        }
    }
}

void ui_cleanup() {
    if (sdlTexture) SDL_DestroyTexture(sdlTexture);
    if (screen) SDL_FreeSurface(screen);
    if (sdlRenderer) SDL_DestroyRenderer(sdlRenderer);
    if (sdlWindow) SDL_DestroyWindow(sdlWindow);
    
    if (sdlDebugTexture) SDL_DestroyTexture(sdlDebugTexture);
    if (debugScreen) SDL_FreeSurface(debugScreen);
    if (sdlDebugRenderer) SDL_DestroyRenderer(sdlDebugRenderer);
    if (sdlDebugWindow) SDL_DestroyWindow(sdlDebugWindow);
    
    TTF_Quit();
    SDL_Quit();
    
    printf("UI cleanup complete\n");
}