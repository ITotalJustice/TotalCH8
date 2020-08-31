// The MIT License (MIT)

// Copyright (c) 2020 TotalJustice

//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.

#define CH8_IMPLEMENTATION
#include "../../../ch8.h"
#define CH8_ROMS_IMPLEMENTATION
#include "../../../ch8_ext_roms.h"

#include <3ds.h>
#include <citro2d.h>

#define WIDTH 400
#define HEIGHT 240
#define SCALE 6

struct Screen {
    C3D_RenderTarget *top;
    C3D_RenderTarget *bottom;
    u32 bg_colour;
    u32 fg_colour;
};

static void sound_cb(void *user_data) {}

static void draw_cb(const struct Ch8_display *display, void *user_data) {
    struct Screen *screen = (struct Screen*)user_data;

    for (u8 y = 0; y < display->height; y++) {
        for (u8 x = 0; x < display->width; x++) {
            const u32 col = ch8_get_pixel(display,x,y) ? screen->fg_colour : screen->bg_colour;
            C2D_DrawRectSolid(x * SCALE, y * SCALE, 0.0, SCALE, SCALE, col);
        }
    }
}

int main(int argc, char **argv) {

    struct Screen screen = {0};
    ch8_t ch8 = {0};
    ch8_init(&ch8, draw_cb, &screen, sound_cb, 0);
    ch8_loadbuiltinrom(&ch8, INVADERS);

    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    screen.top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    // TODO: ui on the bottom
    screen.bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    screen.bg_colour = C2D_Color32(0x00, 0x00, 0x00, 0xFF);
    screen.fg_colour = C2D_Color32(0x00, 0xFF, 0x00, 0xFF);

    while (aptMainLoop()) {
        hidScanInput();
        const u32 kDown = hidKeysDown();
        const u32 kUp = hidKeysUp();

        if (kDown & KEY_START) break;

        // q
        if (kDown & KEY_LEFT) ch8_set_key(&ch8, CH8KEY_4, true);
        else if(kUp & KEY_LEFT) ch8_set_key(&ch8, CH8KEY_4, false);
        // w
        if (kDown & KEY_A) ch8_set_key(&ch8, CH8KEY_5, true);
        else if(kUp & KEY_A) ch8_set_key(&ch8, CH8KEY_5, false);
        // e
        if (kDown & KEY_RIGHT) ch8_set_key(&ch8, CH8KEY_6, true);
        else if(kUp & KEY_RIGHT) ch8_set_key(&ch8, CH8KEY_6, false);

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_SceneBegin(screen.top);

        ch8_run(&ch8);

        C3D_FrameEnd(0);
    }

    C2D_Fini();
    C3D_Fini();
    gfxExit();

    return 0;
}