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
#include "../../../../ch8.h"
#define CH8_ROMS_IMPLEMENTATION
#include "../../../../ch8_ext_roms.h"

#include <stdint.h>
#include <switch.h>

#define WIDTH 1280
#define HEIGHT 640
#define SCALE 20
#define BG_COLOUR RGBA8(0x00,0x00,0x00,0xFF)
#define FG_COLOUR RGBA8(0x00,0xFF,0x00,0xFF)

struct Screen {
    ViDisplay display;
    ViLayer layer;
    NWindow window;
    Framebuffer fb;
    uint32_t *pixels;
};

static void draw_pixel(struct Screen *screen, int x, int y, uint32_t col) {
    const int pos = (y * WIDTH) + x;
    screen->pixels[pos] = col;
}

static void draw_rect(struct Screen *screen, int x, int y, int w, int h, uint32_t col) {
    for (int _y = 0; _y < h; _y++) {
        for (int _x = 0; _x < w; _x++) {
            draw_pixel(screen, _x + x, _y + y, col);
        }  
    }
}

static void sound_cb(void *user_data) {}

static void draw_cb(const struct Ch8_display *display, void *user_data) {
    struct Screen *screen = (struct Screen*)user_data;

    for (uint8_t y = 0; y < display->height; y++) {
        for (uint8_t x = 0; x < display->width; x++) {
            const uint32_t col = ch8_get_pixel(display,x,y) ? FG_COLOUR : BG_COLOUR;
            draw_rect(screen, x * SCALE, y * SCALE, SCALE, SCALE, col);
        }
    }
}

int main(int argc, char **argv) {
    appletLockExit();

    struct Screen screen = {0};
    ch8_t ch8 = {0};
    ch8_init(&ch8, draw_cb, &screen, sound_cb, 0);
    ch8_loadbuiltinrom(&ch8, PONG);

    viInitialize(ViServiceType_Application);
    viOpenDefaultDisplay(&screen.display);
    viCreateLayer(&screen.display, &screen.layer);
    viSetLayerScalingMode(&screen.layer, ViScalingMode_FitToLayer);
    nwindowCreateFromLayer(&screen.window, &screen.layer);
    nwindowSetDimensions(&screen.window, WIDTH, HEIGHT);
    framebufferCreate(&screen.fb, &screen.window, WIDTH, HEIGHT, PIXEL_FORMAT_RGBA_8888, 2);
    framebufferMakeLinear(&screen.fb);

    while (appletMainLoop()) {
        hidScanInput();
        const uint64_t kDown = hidKeysDown(CONTROLLER_P1_AUTO);
        const uint64_t kUp = hidKeysUp(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS) break;

        // q
        if (kDown & KEY_LEFT) ch8_set_key(&ch8, CH8KEY_4, true);
        else if(kUp & KEY_LEFT) ch8_set_key(&ch8, CH8KEY_4, false);
        // w
        if (kDown & KEY_A) ch8_set_key(&ch8, CH8KEY_5, true);
        else if(kUp & KEY_A) ch8_set_key(&ch8, CH8KEY_5, false);
        // e
        if (kDown & KEY_RIGHT) ch8_set_key(&ch8, CH8KEY_6, true);
        else if(kUp & KEY_RIGHT) ch8_set_key(&ch8, CH8KEY_6, false);

        screen.pixels = (uint32_t*)framebufferBegin(&screen.fb, 0);

        ch8_run(&ch8);

        framebufferEnd(&screen.fb);
    }

    framebufferClose(&screen.fb);
    nwindowClose(&screen.window);
    viCloseLayer(&screen.layer);
    viCloseDisplay(&screen.display);
    viExit();

    appletUnlockExit();

    return 0;
}