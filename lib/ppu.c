#include <ppu.h>
#include <lcd.h>
#include <string.h>
#include <ppu_sm.h>

void pipeline_fifo_reset();
void pipeline_process();

static ppu_context context;

ppu_context *ppu_get_context() {
    return &context;
}

void ppu_init() {
    context.current_frame = 0;
    context.line_ticks = 0;
    context.video_buffer = malloc(YRES * XRES * sizeof(32));

    context.pfc.line_x = 0;
    context.pfc.pushed_x = 0;
    context.pfc.fetch_x = 0;
    context.pfc.pixel_fifo.size = 0;
    context.pfc.pixel_fifo.head = context.pfc.pixel_fifo.tail = NULL;
    context.pfc.cur_fetch_state = FS_TILE;

    context.line_sprites = 0;
    context.fetched_entry_count = 0;
    context.window_line = 0;

    lcd_init();
    LCDS_MODE_SET(MODE_OAM);

    memset(context.oam_ram, 0, sizeof(context.oam_ram));
    memset(context.video_buffer, 0, YRES * XRES * sizeof(u32));
}

void ppu_tick() {
    context.line_ticks++;

    switch(LCDS_MODE) {
    case MODE_OAM:
        ppu_mode_oam();
        break;
    case MODE_XFER:
        ppu_mode_xfer();
        break;
    case MODE_VBLANK:
        ppu_mode_vblank();
        break;
    case MODE_HBLANK:
        ppu_mode_hblank();
        break;
    }
}


void ppu_oam_write(u16 address, u8 value) {
    if (address >= 0xFE00) {
        address -= 0xFE00;
    }

    u8 *p = (u8 *)context.oam_ram;
    p[address] = value;
}

u8 ppu_oam_read(u16 address) {
    if (address >= 0xFE00) {
        address -= 0xFE00;
    }

    u8 *p = (u8 *)context.oam_ram;
    return p[address];
}

void ppu_vram_write(u16 address, u8 value) {
    context.vram[address - 0x8000] = value;
}

u8 ppu_vram_read(u16 address) {
    return context.vram[address - 0x8000];
}