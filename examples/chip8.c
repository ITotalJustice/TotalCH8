#define SDL_MAIN_HANDLED
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>

#define CH8_IMPLEMENTATION
#include "../ch8.h"

#define SCALE 10

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stdout, "missing args\n");
        return -1;
    }

    ch8_t *ch8 = ch8_init();
    if (!ch8) {
        fprintf(stdout, "failed to init ch8\n");
        return -1;
    }

    if (ch8_loadrom(ch8, argv[1]) == false) {
        fprintf(stdout, "failed to load rom %s\n", argv[1]);
        return -1;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {
        fprintf(stdout, "failed to init sdl2\n");
        return -1;
    }

    SDL_Window *window = SDL_CreateWindow("chip8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, CH8_DISPLAY_W * SCALE, CH8_DISPLAY_H * SCALE, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_PixelFormat *format = SDL_AllocFormat(SDL_GetWindowPixelFormat(window));
    SDL_Texture *texture = SDL_CreateTexture(renderer, format->format, SDL_TEXTUREACCESS_STREAMING, CH8_DISPLAY_W, CH8_DISPLAY_H);

    const uint32_t fg_colour = SDL_MapRGB(format, 0x00,0xFF,0x00);
    const uint32_t bg_colour = SDL_MapRGB(format, 0x00,0x00,0x00);

    size_t rom_str_size = strlen(argv[1]);
    char *savepath = malloc(rom_str_size + 1 + 7);
    sprintf(savepath, "%s%s", argv[1], ".ch8sv");

    bool quit = false;

    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT: quit = true; break;
            case SDL_KEYDOWN: case SDL_KEYUP:
                switch (e.key.keysym.sym) {
                /// Game inputs.
                case SDLK_1: ch8_input(ch8, CH8KEY_0, e.type == SDL_KEYDOWN); break;
                case SDLK_2: ch8_input(ch8, CH8KEY_1, e.type == SDL_KEYDOWN); break;
                case SDLK_3: ch8_input(ch8, CH8KEY_2, e.type == SDL_KEYDOWN); break;
                case SDLK_4: ch8_input(ch8, CH8KEY_3, e.type == SDL_KEYDOWN); break;
                case SDLK_q: ch8_input(ch8, CH8KEY_4, e.type == SDL_KEYDOWN); break;
                case SDLK_w: ch8_input(ch8, CH8KEY_5, e.type == SDL_KEYDOWN); break;
                case SDLK_e: ch8_input(ch8, CH8KEY_6, e.type == SDL_KEYDOWN); break;
                case SDLK_r: ch8_input(ch8, CH8KEY_7, e.type == SDL_KEYDOWN); break;
                case SDLK_a: ch8_input(ch8, CH8KEY_8, e.type == SDL_KEYDOWN); break;
                case SDLK_s: ch8_input(ch8, CH8KEY_9, e.type == SDL_KEYDOWN); break;
                case SDLK_d: ch8_input(ch8, CH8KEY_A, e.type == SDL_KEYDOWN); break;
                case SDLK_f: ch8_input(ch8, CH8KEY_B, e.type == SDL_KEYDOWN); break;
                case SDLK_z: ch8_input(ch8, CH8KEY_C, e.type == SDL_KEYDOWN); break;
                case SDLK_x: ch8_input(ch8, CH8KEY_D, e.type == SDL_KEYDOWN); break;
                case SDLK_c: ch8_input(ch8, CH8KEY_E, e.type == SDL_KEYDOWN); break;
                case SDLK_v: ch8_input(ch8, CH8KEY_F, e.type == SDL_KEYDOWN); break;

                /// Misc
                case SDLK_k: if (e.type == SDL_KEYUP) ch8_savestate(ch8, savepath); break;
                case SDLK_l: if (e.type == SDL_KEYUP) ch8_loadstate(ch8, savepath); break;
                case SDLK_SPACE: if (e.type == SDL_KEYUP) ch8_pause(ch8); break;
                case SDLK_ESCAPE: if (e.type == SDL_KEYUP) quit = true; break;
                }
                break;
            }
        }

        ch8_run(ch8);

        /// update the gfx.
        if (ch8_update_video(ch8)) {
            uint32_t *pixels; int pitch;
            SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);

            for (uint8_t y = 0; y < CH8_DISPLAY_H; y++) {
                const uint16_t pos = y * CH8_DISPLAY_W;
                for (uint8_t x = 0; x < CH8_DISPLAY_W; x++) {
                    pixels[x + pos] = ch8_get_pixel(ch8,x,y) ? fg_colour : bg_colour;
                }
            }

            SDL_UnlockTexture(texture);
        }

        /// play sound.
        if (ch8_update_sound(ch8)) {
            
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    SDL_FreeFormat(format);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    free(savepath);
    ch8_exit(ch8);

    return 0;
}