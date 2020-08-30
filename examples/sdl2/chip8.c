#define CH8_IMPLEMENTATION
#include "../../ch8.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>

#define SCALE 10

struct Draw {
    u32 fg;
    u32 bg;
    SDL_Texture *texture;
};

static void sound_cb(void *user_data) {

}

static void draw_cb(const struct Ch8_display *display, void *user_data) {
    struct Draw *draw = (struct Draw*)user_data;
    u32 *pixels; int pitch;
    SDL_LockTexture(draw->texture, NULL, (void**)&pixels, &pitch);

    for (u8 y = 0; y < CH8_DISPLAY_H; y++) {
        const u16 pos = y * CH8_DISPLAY_W;
        for (u8 x = 0; x < CH8_DISPLAY_W; x++) {
            pixels[x + pos] = ch8_get_pixel(display,x,y) ? draw->fg : draw->bg;
        }
    }

    SDL_UnlockTexture(draw->texture);
}

static bool loadrom(ch8_t *c, const char *path) {
    if (!c || !path) return NULL;
    FILE *fp = fopen(path, "rb");
    if (!fp) return false;
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    rewind(fp);
    if (!sz || sz > CH8_MAX_ROM_SIZE) goto error_close;
    u8 *data = (u8*)malloc(sz);
    fread((void*)data, sz, 1, fp);
    if (!ch8_loadrom(c, data, (u64)sz)) goto error_free;
    fclose(fp);
    free(data);
    return true;

    error_free:
        free(data);
    error_close:
        fclose(fp);
    return false;
}

static void set_key(ch8_t *ch8, SDL_Keycode key, bool key_down) {

}

int main(int argc, char **argv) {
    if (argc < 2) {
        perror("missing args\n");
        return -1;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0) {
        perror("failed to init sdl2\n");
        return -1;
    }

    SDL_Window *window = SDL_CreateWindow("chip8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, CH8_DISPLAY_W * SCALE, CH8_DISPLAY_H * SCALE, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_PixelFormat *format = SDL_AllocFormat(SDL_GetWindowPixelFormat(window));
    SDL_Texture *texture = SDL_CreateTexture(renderer, format->format, SDL_TEXTUREACCESS_STREAMING, CH8_DISPLAY_W, CH8_DISPLAY_H);

    const u32 fg_colour = SDL_MapRGB(format, 0x00,0xFF,0x00);
    const u32 bg_colour = SDL_MapRGB(format, 0x00,0x00,0x00);

    struct Draw draw = {
        fg_colour,
        bg_colour,
        texture
    };

    static ch8_t ch8;
    if (!ch8_init(&ch8, draw_cb, &draw, sound_cb, 0)) {
        perror("failed to init ch8\n");
        return -1;
    }

    if (loadrom(&ch8, argv[1]) == false) {
        perror("failed to load rom\n");
        return -1;
    }

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