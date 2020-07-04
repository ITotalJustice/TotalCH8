#ifndef CH8_INCLUDE
#define CH8_INCLUDE

#include <stdint.h>
#include <stdbool.h>

/// Sizes.
#define SPRITE_SIZE 0x5
#define SPRITES_SIZE 0x50
#define CH8_VREG_SIZE 0x10
#define CH8_RESERVED 0x200
#define CH8_RAM_SIZE 0x1000
#define CH8_STACK_SIZE 0x10
#define CH8_MAX_ROM_SIZE CH8_RAM_SIZE - 0x200

/// Chip8 specific.
#define CH8_DISPLAY_W 64
#define CH8_DISPLAY_H 32
#define CH8_DISPLAY_SIZE (CH8_DISPLAY_W * CH8_DISPLAY_H)

/// Defaults.
#define CH8_VSYNC true
#define CH8_WRAP_X false
#define CH8_WRAP_Y false
#define CH8_CLOCK_FREQ 600.0
#define CH8_REFRESH_RATE 60.0
#define CH8_CLOCK_STEP CH8_CLOCK_FREQ / CH8_REFRESH_RATE

typedef enum {
    CH8KEY_0, CH8KEY_1, CH8KEY_2, CH8KEY_3,
    CH8KEY_4, CH8KEY_5, CH8KEY_6, CH8KEY_7,
    CH8KEY_8, CH8KEY_9, CH8KEY_A, CH8KEY_B,
    CH8KEY_C, CH8KEY_D, CH8KEY_E, CH8KEY_F,
} CH8_Key;

struct Ch8_reg {
    uint8_t v[CH8_VREG_SIZE]; /// general register
    uint8_t st;      /// sound timer
    uint8_t dt;      /// delay timer
    uint16_t i:12;   /// index.
    uint16_t pc:12;  /// program counter
    uint16_t sp:12;  /// stack pointer.
};

struct Ch8_mem {
    uint8_t ram[CH8_RAM_SIZE];
    uint16_t stack[CH8_STACK_SIZE];
};

struct Ch8_display {
    bool draw;
    bool wrap;
    bool pixels[CH8_DISPLAY_SIZE];
    uint16_t width, height, size;
};

struct Ch8_input {
    bool keys[0x10];
};

struct Ch8_settings {
    bool pause;
    float clock_freq;
    float clock_step;
};

struct Ch8_var {
    uint8_t n:4;
    uint8_t x:4;
    uint8_t y:4;
    uint8_t kk;
    uint16_t nnn:12;
    uint16_t opcode;
};

typedef struct {
    struct Ch8_reg reg;
    struct Ch8_mem mem;
    struct Ch8_display display;
    struct Ch8_input input;
    struct Ch8_settings set;
    struct Ch8_var var;
} ch8_t;

/// API CALLS
void ch8_input(ch8_t *ch8, CH8_Key key, bool pressed);
bool ch8_update_video(ch8_t *ch8);
bool ch8_update_sound(const ch8_t *ch8);
void ch8_wrap(ch8_t *ch8, bool enabled);
bool ch8_is_wrapped(const ch8_t *ch8);
bool ch8_savestate(ch8_t *ch8, const char *filepath);
bool ch8_loadstate(ch8_t *ch8, const char *filepath);
bool ch8_loadrom(ch8_t *ch8, const char *path);
void ch8_reset(ch8_t *ch8);
bool ch8_get_pixel(const ch8_t *ch8, uint8_t x, uint8_t y);
void ch8_pause(ch8_t *ch8);
void ch8_run(ch8_t *ch8);
ch8_t *ch8_init(void);
void ch8_exit(ch8_t *ch8);

#ifdef CH8_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// Helper macros.
#define CH8_opcode  ch8->var.opcode
#define CH8_ram     ch8->mem.ram
#define CH8_stack   ch8->mem.stack
#define CH8_PC      ch8->reg.pc
#define CH8_SP      ch8->reg.sp
#define CH8_I       ch8->reg.i
#define CH8_V       ch8->reg.v
#define CH8_DT      ch8->reg.st
#define CH8_ST      ch8->reg.st
#define CH8_pixels  ch8->display.pixels
#define CH8_width   ch8->display.width
#define CH8_height  ch8->display.height
#define CH8_size    ch8->display.size
#define CH8_wrap    ch8->display.wrap
#define CH8_draw    ch8->display.draw
#define CH8_keys    ch8->input.keys
#define CH8_nnn     ch8->var.nnn
#define CH8_kk      ch8->var.kk
#define CH8_n       ch8->var.n
#define CH8_x       ch8->var.x
#define CH8_y       ch8->var.y
#define MEMCLR(ptr,size) do memset(ptr, 0, size); while(0)

/// Instructions.
#define CLS() do { MEMCLR(CH8_pixels, sizeof(CH8_pixels)); CH8_draw = true; } while(0)
#define SE(a,b) do if (a == b) CH8_PC += 2; while (0)
#define SNE(a,b) do if (a != b) CH8_PC += 2; while (0)
#define SKP(v) do if (CH8_keys[v]) CH8_PC += 2; while (0)
#define SKNP(v) do if (!CH8_keys[v]) CH8_PC += 2; while (0)
#define RET() do CH8_PC = CH8_stack[CH8_SP--]; while(0)
#define CALL(v) do { CH8_stack[++CH8_SP] = CH8_PC; CH8_PC = v; } while(0)
#define JMP(v) do CH8_PC = v; while(0)
#define LD(a,b) do a = b; while(0)
#define AND(a,b) do a &= b; while(0)
#define ADD(a,b) do a += b; while(0)
#define ADC(a,b) do { CH8_V[0xF] = (a + b) > 0xFF; ADD(a,b); } while(0)
#define SUB(a,b) do a -= b; while(0)
#define SBC(a,b) do { CH8_V[0xF] = a > b; SUB(a,b); } while(0)
#define OR(a,b) do a |= b; while(0)
#define XOR(a,b) do a ^= b; while(0)
#define SHL(a) do { CH8_V[0xF] = (a >> 7) & 1; a <<= 1; } while(0)
#define SHR(a) do { CH8_V[0xF] = a & 1; a >>= 1; } while(0)
#define RND(a,mask) do a = (rand() % 0xFF) & mask; while(0)
#define DRAW() do { \
    CH8_V[0xF] = 0; \
    for (uint8_t n = 0; n < CH8_n; n++) { \
    const uint16_t pos = CH8_V[CH8_x] + ((CH8_V[CH8_y] + n) * CH8_width); \
        for (uint8_t bit = 0; bit < 8; bit++) { \
            if (CH8_ram[CH8_I + n] & (0x80 >> bit)) { \
                uint16_t bitpos = pos + bit; \
                if (CH8_wrap == true) \
                    bitpos %= CH8_width; \
                CH8_V[0xF] |= CH8_pixels[bitpos]; \
                CH8_pixels[bitpos] ^= 1; \
            } \
        } \
    } \
    CH8_draw = true; \
} while (0)

/// Insturction groups.
#define _0000() do { \
    switch (CH8_opcode & 0x00FF) { \
        case 0x00E0: CLS(); break; \
        case 0x00EE: RET(); break; \
    } \
} while(0)
#define _1000() do JMP(CH8_nnn); while(0)
#define _2000() do CALL(CH8_nnn); while(0)
#define _3000() do SE(CH8_V[CH8_x], CH8_kk); while(0)
#define _4000() do SNE(CH8_V[CH8_x], CH8_kk); while(0)
#define _5000() do SE(CH8_V[CH8_x], CH8_V[CH8_y]); while(0)
#define _6000() do LD(CH8_V[CH8_x], CH8_kk); while(0)
#define _7000() do ADD(CH8_V[CH8_x], CH8_kk); while(0)
#define _8000() do { \
    switch (CH8_opcode & 0x000F) { \
        case 0x0: LD(CH8_V[CH8_x], CH8_V[CH8_y]); break; \
        case 0x1: OR(CH8_V[CH8_x], CH8_V[CH8_y]); break; \
        case 0x2: AND(CH8_V[CH8_x], CH8_V[CH8_y]); break; \
        case 0x3: XOR(CH8_V[CH8_x], CH8_V[CH8_y]); break; \
        case 0x4: ADC(CH8_V[CH8_x], CH8_V[CH8_y]); break; \
        case 0x5: SBC(CH8_V[CH8_x], CH8_V[CH8_y]); break; \
        case 0x6: SHR(CH8_V[CH8_x]); break; \
        case 0x7: SBC(CH8_V[CH8_y], CH8_V[CH8_x]); break; \
        case 0xE: SHL(CH8_V[CH8_x]); break; \
    } \
} while(0)
#define _9000() do SNE(CH8_V[CH8_x], CH8_V[CH8_y]); while(0)
#define _A000() do LD(CH8_I, CH8_nnn); while(0)
#define _B000() do JMP(CH8_V[0] + CH8_nnn); while(0)
#define _C000() do RND(CH8_V[CH8_x], CH8_kk); while(0)
#define _D000() do DRAW(); while(0)
#define _E000() do { \
    switch (CH8_opcode & 0x00FF) { \
        case 0x009E: SKP(CH8_V[CH8_x]); break; \
        case 0x00A1: SKNP(CH8_V[CH8_x]); break; \
    } \
} while(0)
#define _F000() do { \
    switch (CH8_opcode & 0x00FF) { \
        case 0x07: LD(CH8_V[CH8_x], CH8_DT); break; \
        case 0x15: LD(CH8_DT, CH8_V[CH8_x]); break; \
        case 0x18: LD(CH8_ST, CH8_V[CH8_x]); break; \
        case 0x1E: ADD(CH8_I, CH8_V[CH8_x]); break; \
        case 0x29: LD(CH8_I, CH8_V[CH8_x] * 5); break; \
        case 0x33: \
            LD(CH8_ram[CH8_I], (CH8_V[CH8_x] % 1000) / 100); \
            LD(CH8_ram[CH8_I + 1], (CH8_V[CH8_x] % 100) / 10); \
            LD(CH8_ram[CH8_I + 2], (CH8_V[CH8_x] % 10) / 1); \
            break; \
        case 0x55: \
            for (uint8_t i = 0; i <= CH8_x; i++) \
                LD(CH8_ram[CH8_I + i], CH8_V[i]); \
            break; \
        case 0x65: \
            for (uint8_t i = 0; i <= CH8_x; i++) \
                LD(CH8_V[i], CH8_ram[CH8_I + i]); \
            break; \
    } \
} while(0)

#define FETCH() do { \
    CH8_opcode = (CH8_ram[CH8_PC] << 8) | CH8_ram[CH8_PC + 1]; \
    CH8_nnn = CH8_opcode & 0x0FFF; \
    CH8_n = CH8_opcode & 0x000F; \
    CH8_x = (CH8_opcode & 0x0F00) >> 8; \
    CH8_y = (CH8_opcode & 0x00F0) >> 4; \
    CH8_kk = CH8_opcode & 0x00FF; \
    CH8_PC += 2; \
} while(0)

#define EXECUTE() do { \
    switch ((CH8_opcode & 0xF000) >> 12) { \
        case 0x0: _0000(); break; \
        case 0x1: _1000(); break; \
        case 0x2: _2000(); break; \
        case 0x3: _3000(); break; \
        case 0x4: _4000(); break; \
        case 0x5: _5000(); break; \
        case 0x6: _6000(); break; \
        case 0x7: _7000(); break; \
        case 0x8: _8000(); break; \
        case 0x9: _9000(); break; \
        case 0xA: _A000(); break; \
        case 0xB: _B000(); break; \
        case 0xC: _C000(); break; \
        case 0xD: _D000(); break; \
        case 0xE: _E000(); break; \
        case 0xF: _F000(); break; \
    } \
} while(0)


/// API
bool ch8_savestate(ch8_t *ch8, const char *filepath) {
    FILE *fp = fopen(filepath, "wb");
    if (!fp) goto fail;

    ch8_t *temp_ch8 = malloc(sizeof(ch8_t));
    if (!temp_ch8) goto fail_close;

    if (!memcpy(temp_ch8, ch8, sizeof(ch8_t))) goto fail_free;

    if (!fwrite(temp_ch8, sizeof(ch8_t), 1, fp)) goto fail_free;

    free(temp_ch8);
    fclose(fp);
    return true;

    fail_free:
        free(temp_ch8);
    fail_close:
        fclose(fp);
    fail:
        return false;
}

bool ch8_loadstate(ch8_t *ch8, const char *filepath) {
    FILE *fp = fopen(filepath, "rb");
    if (!fp) goto fail;

    ch8_t *temp_ch8 = malloc(sizeof(ch8_t));
    if (!temp_ch8) goto fail_close;

    if (!fread(temp_ch8, sizeof(ch8_t), 1, fp)) goto fail_free;

    if (!memcpy(ch8, temp_ch8, sizeof(ch8_t))) goto fail_free;

    free(temp_ch8);
    fclose(fp);
    return true;

    fail_free:
        free(temp_ch8);
    fail_close:
        fclose(fp);
    fail:
        return false;
}

void ch8_reset(ch8_t *ch8) {
    MEMCLR(&ch8->reg, sizeof(ch8->reg));
    MEMCLR(&ch8->var, sizeof(ch8->var));
    MEMCLR(&ch8->mem, sizeof(ch8->mem));
    MEMCLR(&ch8->input, sizeof(ch8->input));
    MEMCLR(&ch8->display, sizeof(ch8->display));
    MEMCLR(&ch8->set, sizeof(ch8->set));

    const uint8_t CH8_FONTSET[SPRITES_SIZE] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, /// 0x0
        0x20, 0x60, 0x20, 0x20, 0x70, /// 0x1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, /// 0x2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, /// 0x3
        0x90, 0x90, 0xF0, 0x10, 0x10, /// 0x4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, /// 0x5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, /// 0x6
        0xF0, 0x10, 0x20, 0x40, 0x40, /// 0x7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, /// 0x8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, /// 0x9
        0xF0, 0x90, 0xF0, 0x90, 0x90, /// 0xA
        0xE0, 0x90, 0xE0, 0x90, 0xE0, /// 0xB
        0xF0, 0x80, 0x80, 0x80, 0xF0, /// 0xC
        0xE0, 0x90, 0x90, 0x90, 0xE0, /// 0xD
        0xF0, 0x80, 0xF0, 0x80, 0xF0, /// 0xE
        0xF0, 0x80, 0xF0, 0x80, 0x80  /// 0xF 
    };

    memcpy(CH8_ram, CH8_FONTSET, 0x50);
    CH8_PC = 0x200;

    CH8_width = CH8_DISPLAY_W;
    CH8_height = CH8_DISPLAY_H;
    CH8_size = CH8_DISPLAY_SIZE;

    ch8->set.clock_freq = CH8_CLOCK_FREQ;
    ch8->set.clock_step = CH8_CLOCK_STEP;
}

ch8_t *ch8_init(void) {
    ch8_t *ch8 = malloc(sizeof(ch8_t));
    if (!ch8) {
        return NULL;
    }

    return ch8;
}

void ch8_exit(ch8_t *ch8) {
    if (!ch8) return;
    
    free(ch8);
}

bool ch8_loadrom(ch8_t *ch8, const char *path) {
    ch8_reset(ch8);

    FILE *fp = fopen(path, "rb");
    if (!fp) goto fail;

    fseek(fp, 0, SEEK_END);
    const uint16_t size = ftell(fp);
    rewind(fp);
    if (!size || size > CH8_MAX_ROM_SIZE) goto fail_close;

    if (!fread(CH8_ram + 0x200, size, 1, fp)) goto fail_close;

    return true;

    fail_close:
        fclose(fp);
    fail:
        return false;
}

bool ch8_update_video(ch8_t *ch8) {
    const bool up_vid = CH8_draw;
    CH8_draw = false;
    return up_vid;
}

bool ch8_update_sound(const ch8_t *ch8) {
    return CH8_ST > 0;
}

void ch8_wrap(ch8_t *ch8, bool enabled) {
    CH8_wrap = enabled;
}

bool ch8_is_wrapped(const ch8_t *ch8) {
    return CH8_wrap;
}

void ch8_input(ch8_t *ch8, CH8_Key key, bool pressed) {
    CH8_keys[key] = pressed;
}

bool ch8_get_pixel(const ch8_t *ch8, uint8_t x, uint8_t y) {
    return CH8_pixels[x + (CH8_width * y)];
}

void ch8_pause(ch8_t *ch8) {
    ch8->set.pause = !ch8->set.pause;
}

void ch8_run(ch8_t *ch8) {
    if (ch8->set.pause || CH8_draw){
        return;
    }

    const float freq = ch8->set.clock_freq;
    const float step = ch8->set.clock_step;

    for (float i = 0.0; i < freq && !CH8_draw; i += step) {
        FETCH();
        EXECUTE();
    }

    if (CH8_DT) {
        CH8_DT--;
    }

    if (CH8_ST) {
        CH8_ST--;
    }
}
#endif /* CH8_IMPLEMENTATION */
#endif /* CH8_INCLUDE */