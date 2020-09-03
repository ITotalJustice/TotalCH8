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

// based on https://github.com/devkitPro/nds-examples/tree/master/Graphics/gl2d/dual_screen/source

#define ARM9 // for vscode
#include <nds.h>
#include <gl2d.h>

#define WIDTH 256
#define HEIGHT 192
#define SCALE 4

int main(int argc, char **argv) {
    ch8_t ch8 = {0};
    ch8_init(&ch8);
    ch8_loadbuiltinrom(&ch8, INVADERS);

	videoSetMode(MODE_5_3D);
	videoSetModeSub(MODE_5_2D);
 
	// init oam to capture 3D scene
    oamInit(&oamSub, SpriteMapping_Bmp_2D_256, false);
	for (int y = 0, id = 0; y < 3; y++) {
        for (int x = 0; x < 4; x++) {
            oamSub.oamMemory[id].attribute[0] = ATTR0_BMP | ATTR0_SQUARE | (64 * y);
            oamSub.oamMemory[id].attribute[1] = ATTR1_SIZE_64 | (64 * x);
            oamSub.oamMemory[id].attribute[2] = ATTR2_ALPHA(1) | (8 * 32 * y) | (8 * x);
            id++;
        }
    }
	swiWaitForVBlank();
	oamUpdate(&oamSub);

    bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
 
	glScreen2D();
	
    uint8_t screen_idx = 0;

	while (1) {
		scanKeys();
		const uint32 kDown = keysDown();
        const uint32 kUp = keysUp();

        // q
        if (kDown & KEY_LEFT) ch8_set_key(&ch8, CH8KEY_4, true);
        else if(kUp & KEY_LEFT) ch8_set_key(&ch8, CH8KEY_4, false);
        // w
        if (kDown & KEY_A) ch8_set_key(&ch8, CH8KEY_5, true);
        else if(kUp & KEY_A) ch8_set_key(&ch8, CH8KEY_5, false);
        // e
        if (kDown & KEY_RIGHT) ch8_set_key(&ch8, CH8KEY_6, true);
        else if(kUp & KEY_RIGHT) ch8_set_key(&ch8, CH8KEY_6, false);
	
        // alternate screen
		if ((screen_idx ^= 1)) {
            lcdMainOnBottom();
            vramSetBankC(VRAM_C_LCD);
            vramSetBankD(VRAM_D_SUB_SPRITE);
            REG_DISPCAPCNT = DCAP_BANK(2) | DCAP_ENABLE | DCAP_SIZE(3);
        } else {
            lcdMainOnTop();
            vramSetBankD(VRAM_D_LCD);
            vramSetBankC(VRAM_C_SUB_BG);
            REG_DISPCAPCNT = DCAP_BANK(3) | DCAP_ENABLE | DCAP_SIZE(3);
        }

        ch8_run(&ch8);

        static const int xoff = (WIDTH - CH8_DISPLAY_W * SCALE) / 2;
        static const int yoff = (HEIGHT - CH8_DISPLAY_H * SCALE) / 2;

        glBegin2D();
            for (uint8_t y = 0; y < CH8_DISPLAY_H; y++) {
                for (uint8_t x = 0; x < CH8_DISPLAY_W; x++) {
                    if (ch8_get_pixel(&ch8,x,y)) {
                        const int px = xoff + x * SCALE; 
                        const int py = yoff + y * SCALE;
                        const int px2 = px + SCALE;
                        const int py2 = py + SCALE;
                        glBoxFilled(px, py, px2, py2, RGB15(0,0xFF,0));
                    }
                }
            }
        glEnd2D();
		
		glFlush(0);
	}

	return 0;
}
