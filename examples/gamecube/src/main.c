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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <gccore.h>

// Heavily based on usbloaderGX
// https://github.com/cyan06/usbloadergx/blob/master/source/video.cpp

#define DEFAULT_FIFO_SIZE (256 * 1024)
#define WIDTH 640
#define HEIGHT 480
#define SCALE 10

struct Screen {
    uint32_t width, height;
    GXColor bg_colour, fg_colour;
	uint8_t fb_idx;
    uint32_t *fb[2];
	Mtx44 projection2d;
	Mtx modleview2d;
	GXRModeObj *vmode;
};

// helpers. not all are used, but i plan to use them at some point.
static inline uint32_t RGBAgx32(GXColor col) {
	return ((col.r << 24) | (col.g << 16) | (col.b << 8) | col.a);
}
static inline GXColor gxRGAB32(uint32_t col) {
	const GXColor gxcol = { .r = col >> 24, .g = col >> 16, .b = col >> 8, .a = col };
	return gxcol;
}
static inline GXColor gxRGBA8(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	const GXColor gxcol = { .r = r, .g = g, .b = b, .a = a };
	return gxcol;
}
static inline GXColor gxRGB8(uint8_t r, uint8_t g, uint8_t b) {
	const GXColor gxcol = { .r = r, .g = g, .b = b, .a = 0xFF };
	return gxcol;
}

static void draw_rect(struct Screen *screen, float x, float y, float w, float h, GXColor col) {
	GX_LoadProjectionMtx(screen->projection2d, GX_ORTHOGRAPHIC);
	GX_LoadPosMtxImm(screen->modleview2d, GX_PNMTX0);

	GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
	GX_ClearVtxDesc();
	GX_InvVtxCache();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_NONE);

	const float x2 = x + w;
	const float y2 = y + h;
	const uint16_t vert = 4; // rect has 4 sides :)
	const guVector v[] = {
		{ .x = x, .y = y, .z = 0.0f },
		{ .x = x2, .y = y, .z = 0.0f },
		{ .x = x2, .y = y2, .z = 0.0f },
		{ .x = x, .y = y2, .z = 0.0f },
		{ .x = x, .y = y, .z = 0.0f }
	};

	GX_Begin(GX_TRIANGLEFAN, GX_VTXFMT0, vert);
	for (uint8_t i = 0; i < vert; i++) {
		GX_Position3f32(v[i].x, v[i].y, v[i].z);
		GX_Color1u32(RGBAgx32(col));
	}
	GX_End();
	GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
}

static void sound_cb(void *user_data) {}

static void draw_cb(const struct Ch8_display *display, void *user_data) {
    struct Screen *screen = (struct Screen*)user_data;
	const float yoff = (screen->height - (CH8_DISPLAY_H * SCALE)) / 2;
	const float xoff = (screen->width - (CH8_DISPLAY_W * SCALE)) / 2;

    for (uint8_t y = 0; y < display->height; y++) {
        for (uint8_t x = 0; x < display->width; x++) {
            const GXColor col = ch8_get_pixel(display,x,y) ? screen->fg_colour : screen->bg_colour;
			draw_rect(screen, xoff + (x * SCALE), yoff + (y * SCALE), SCALE, SCALE, col);
        }
    }
}

int main(int argc, char **argv) {
    struct Screen screen = {0};
	screen.width = WIDTH;
	screen.height = HEIGHT;
	screen.bg_colour = gxRGB8(0x00, 0x00, 0x00);
	screen.fg_colour = gxRGB8(0x00, 0xFF, 0x00);

    ch8_t ch8 = {0};
    ch8_init(&ch8, draw_cb, &screen, sound_cb, 0);
    ch8_loadbuiltinrom(&ch8, INVADERS);

	VIDEO_Init();
	VIDEO_SetBlack(FALSE);
	screen.vmode = VIDEO_GetPreferredMode(NULL);
	screen.vmode->viWidth = WIDTH;
	screen.vmode->viXOrigin = (VI_MAX_WIDTH_NTSC - screen.vmode->viWidth) / 2;
	VIDEO_Configure(screen.vmode);

	screen.fb[0] = (uint32_t*)MEM_K0_TO_K1(SYS_AllocateFramebuffer(screen.vmode));
	screen.fb[1] = (uint32_t*)MEM_K0_TO_K1(SYS_AllocateFramebuffer(screen.vmode));

	VIDEO_ClearFrameBuffer(screen.vmode, screen.fb[0], COLOR_BLACK);
	VIDEO_ClearFrameBuffer(screen.vmode, screen.fb[1], COLOR_BLACK);
	VIDEO_SetNextFramebuffer(screen.fb[0]);

	VIDEO_Flush();
	while (VIDEO_GetNextField()) VIDEO_WaitVSync();

	uint8_t *gp_fifo = (uint8_t*)memalign(32, DEFAULT_FIFO_SIZE);
	memset(gp_fifo, 0, DEFAULT_FIFO_SIZE);
	GX_Init(gp_fifo, DEFAULT_FIFO_SIZE);
	GX_SetCopyClear(screen.bg_colour, 0x00FFFFFF);
	GX_SetDispCopyGamma(GX_GM_1_0);
	GX_SetCullMode(GX_CULL_NONE);

	const float yscale = GX_GetYScaleFactor(screen.vmode->efbHeight, screen.vmode->xfbHeight);
	const uint32_t xfbHeight = GX_SetDispCopyYScale(yscale);
	GX_SetScissor(0, 0, screen.vmode->fbWidth, screen.vmode->efbHeight);
	GX_SetDispCopySrc(0, 0, screen.vmode->fbWidth, screen.vmode->efbHeight);
	GX_SetDispCopyDst(screen.vmode->fbWidth, xfbHeight);
	GX_SetCopyFilter(screen.vmode->aa, screen.vmode->sample_pattern, GX_TRUE, screen.vmode->vfilter);
	GX_SetFieldMode(screen.vmode->field_rendering, ((screen.vmode->viHeight == 2 * screen.vmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));
	GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);

	GX_ClearVtxDesc();
	GX_InvVtxCache();
	GX_InvalidateTexAll();

	GX_SetVtxDesc(GX_VA_TEX0, GX_NONE);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);

	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

	for (uint8_t i = 0; i < 8; i++) GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0 + i, GX_TEX_ST, GX_F32, 0);

	GX_SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);

	GX_SetNumChans(1);
	GX_SetNumTexGens(1);
	GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

	guMtxIdentity(screen.modleview2d);
	guMtxTransApply(screen.modleview2d, screen.modleview2d, 0.0f, 0.0f, -9900.0f);
	guOrtho(screen.projection2d, 0, screen.height - 1, 0, screen.width - 1, 0, 10000);

	GX_SetViewport(0.0f, 0.0f, screen.vmode->fbWidth, screen.vmode->efbHeight, 0.0f, 1.0f);
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	GX_SetColorUpdate(GX_TRUE);
	GX_SetAlphaUpdate(GX_TRUE);

	PAD_Init();

	while (1) {
		PAD_ScanPads();
		const uint16_t kUp = PAD_ButtonsUp(0);
		const uint16_t kDown = PAD_ButtonsDown(0);
		
		// q
        if (kDown & PAD_BUTTON_LEFT) ch8_set_key(&ch8, CH8KEY_4, true);
        else if(kUp & PAD_BUTTON_LEFT) ch8_set_key(&ch8, CH8KEY_4, false);
        // w
        if (kDown & PAD_BUTTON_A) ch8_set_key(&ch8, CH8KEY_5, true);
        else if(kUp & PAD_BUTTON_A) ch8_set_key(&ch8, CH8KEY_5, false);
        // e
        if (kDown & PAD_BUTTON_RIGHT) ch8_set_key(&ch8, CH8KEY_6, true);
        else if(kUp & PAD_BUTTON_RIGHT) ch8_set_key(&ch8, CH8KEY_6, false);
		
		ch8_run(&ch8);

		screen.fb_idx ^= 1;
		GX_CopyDisp(screen.fb[screen.fb_idx], GX_TRUE);
		GX_DrawDone();
		VIDEO_SetNextFramebuffer(screen.fb[screen.fb_idx]);
		VIDEO_Flush();
		VIDEO_WaitVSync();
	}

	// not sure if this needs freeing
	free(gp_fifo);

	return 0;
}