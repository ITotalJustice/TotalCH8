#define CH8_IMPLEMENTATION
#include "../../ch8.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>

#define SCALE 10

struct Screen {
    uint32_t fg_colour;
    uint32_t bg_colour;
    SDL_Texture *texture;
};

int main(int argc, char **argv) {

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS);

    SDL_Window *window = SDL_CreateWindow("chip8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, CH8_DISPLAY_W * SCALE, CH8_DISPLAY_H * SCALE, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_PixelFormat *format = SDL_AllocFormat(SDL_GetWindowPixelFormat(window));
    SDL_Texture *texture = SDL_CreateTexture(renderer, format->format, SDL_TEXTUREACCESS_STREAMING, CH8_DISPLAY_W, CH8_DISPLAY_H);

    const uint32_t fg_colour = SDL_MapRGB(format, 0x00,0xFF,0x00);
    const uint32_t bg_colour = SDL_MapRGB(format, 0x00,0x00,0x00);

    struct Screen screen = {
        fg_colour,
        bg_colour,
        texture
    };

    static ch8_t ch8;
    ch8_init(&ch8);

    bool quit = false;
    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT: quit = true; break;
            case SDL_KEYDOWN: case SDL_KEYUP:
                switch (e.key.keysym.sym) {
                    case SDLK_1: ch8_set_key(&ch8, CH8KEY_0, e.type == SDL_KEYDOWN); break;
                    case SDLK_2: ch8_set_key(&ch8, CH8KEY_1, e.type == SDL_KEYDOWN); break;
                    case SDLK_3: ch8_set_key(&ch8, CH8KEY_2, e.type == SDL_KEYDOWN); break;
                    case SDLK_4: ch8_set_key(&ch8, CH8KEY_3, e.type == SDL_KEYDOWN); break;
                    case SDLK_q: ch8_set_key(&ch8, CH8KEY_4, e.type == SDL_KEYDOWN); break;
                    case SDLK_w: ch8_set_key(&ch8, CH8KEY_5, e.type == SDL_KEYDOWN); break;
                    case SDLK_e: ch8_set_key(&ch8, CH8KEY_6, e.type == SDL_KEYDOWN); break;
                    case SDLK_r: ch8_set_key(&ch8, CH8KEY_7, e.type == SDL_KEYDOWN); break;
                    case SDLK_a: ch8_set_key(&ch8, CH8KEY_8, e.type == SDL_KEYDOWN); break;
                    case SDLK_s: ch8_set_key(&ch8, CH8KEY_9, e.type == SDL_KEYDOWN); break;
                    case SDLK_d: ch8_set_key(&ch8, CH8KEY_A, e.type == SDL_KEYDOWN); break;
                    case SDLK_f: ch8_set_key(&ch8, CH8KEY_B, e.type == SDL_KEYDOWN); break;
                    case SDLK_z: ch8_set_key(&ch8, CH8KEY_C, e.type == SDL_KEYDOWN); break;
                    case SDLK_x: ch8_set_key(&ch8, CH8KEY_D, e.type == SDL_KEYDOWN); break;
                    case SDLK_c: ch8_set_key(&ch8, CH8KEY_E, e.type == SDL_KEYDOWN); break;
                    case SDLK_v: ch8_set_key(&ch8, CH8KEY_F, e.type == SDL_KEYDOWN); break;
                    case SDLK_ESCAPE: if (e.type == SDL_KEYUP) quit = true; break;
                }

            }
        }
        ch8_run(&ch8);

        uint32_t *pixels; int pitch;
        SDL_LockTexture(screen.texture, NULL, (void**)&pixels, &pitch);
        for (uint8_t y = 0; y < CH8_DISPLAY_H; y++) {
            const uint16_t pos = y * CH8_DISPLAY_W;
            for (uint8_t x = 0; x < CH8_DISPLAY_W; x++) {
                pixels[x + pos] = ch8_get_pixel(&ch8,x,y) ? screen.fg_colour : screen.bg_colour;
            }
        }
        SDL_UnlockTexture(screen.texture);

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    SDL_FreeFormat(format);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}