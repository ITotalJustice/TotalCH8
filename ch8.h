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

#ifndef CH8_INCLUDE
#define CH8_INCLUDE

#ifndef bool
#define bool _Bool
#endif
#ifdef true
#undef true
#endif
#define true ((bool)1)
#ifdef false
#undef false
#endif
#define false ((bool)0)
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;

/// API
#define CH8_VERSION_NUM_MAJOR 1
#define CH8_VERSION_NUM_MINOR 0
#define CH8_VERSION_NUM_MACRO 0
#define CH8_VERSION_NUM (CH8_VERSION_MAJOR << 24 | CH8_VERSION_MINOR << 16 | CH8_VERSION_MACRO << 8)

#define CH8_VERSION_MAJOR "1"
#define CH8_VERSION_MINOR "0"
#define CH8_VERSION_MACRO "0"
#define CH8_VERSION CH8_VERSION_MAJOR "." CH8_VERSION_MINOR "." CH8_VERSION_MACRO

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
    u8 v[CH8_VREG_SIZE]; /// general register
    u8 st;      /// sound timer
    u8 dt;      /// delay timer
    u16 i:12;   /// index.
    u16 pc:12;  /// program counter
    u16 sp:12;  /// stack pointer.
};

struct Ch8_mem {
    u8 ram[CH8_RAM_SIZE];
    u16 stack[CH8_STACK_SIZE];
};

struct Ch8_display {
    bool draw;
    bool wrap;
    u8 pixels[CH8_DISPLAY_SIZE];
    u8 width, height;
    u16 size;
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
    u8 n:4;
    u8 x:4;
    u8 y:4;
    u8 kk;
    u16 nnn:12;
    u16 opcode;
};

struct ch8_savestate {
    u32 version;
    u32 seed;
    struct Ch8_reg reg;
    u8 ram[CH8_RESERVED-SPRITES_SIZE];
    u16 stack[CH8_STACK_SIZE];
    u8 display[CH8_DISPLAY_SIZE];
};

typedef void (*ch8_cb_draw)(const struct Ch8_display *display, void *user_data);
typedef void (*ch8_cb_sound)(void *user_data);

typedef struct {
    struct Ch8_reg reg;
    struct Ch8_mem mem;
    struct Ch8_display display;
    struct Ch8_input input;
    struct Ch8_settings set;
    struct Ch8_var var;
    struct {
        void *user_data;
        ch8_cb_draw cb;
    } draw;
    struct {
        void *user_data;
        ch8_cb_sound cb;
    } sound;
    u32 seed;
} ch8_t;

/// API CALLS
void ch8_set_key(ch8_t *ch8, CH8_Key key, bool pressed);
bool ch8_loadrom(ch8_t *ch8, const u8 *data, u32 len);
bool ch8_savestate(const ch8_t *ch8, struct ch8_savestate *out_state);
bool ch8_loadstate(ch8_t *ch8, const struct ch8_savestate *state);
void ch8_reset(ch8_t *ch8);
bool ch8_get_pixel(const struct Ch8_display *display, u8 x, u8 y);
void ch8_run(ch8_t *ch8);
bool ch8_init(ch8_t *ch8_out, ch8_cb_draw drw, void *drw_data, ch8_cb_sound snd, void *snd_data);

#ifdef CH8_IMPLEMENTATION

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

static inline void __ch8_memset(void *p, const int v, u32 len) {
    u8 *ptr = (u8*)p;
    u8 value = (u8)v;
    while (len--) *ptr++ = value;
}

static inline void __ch8_memcpy(void *dst, const void *src, u32 len) {
    u8 *d = (u8*)dst;
    const u8 *s = (const u8*)src;
    while (len--) *d++ = *s++;
}

#define MEMSET(p,v,len) __ch8_memset((void*)(p),(v),(len))
#define MEMCPY(d,s,len) __ch8_memcpy((void*)(d),(const void*)(s),(len))

static inline u8 __ch8_rand(ch8_t *ch8) {
    ch8->seed = (214013*ch8->seed+2531011);
    return ((ch8->seed>>16)&0x7FFF) & 0xFF;
}

static void __ch8_reset(ch8_t *ch8, const bool full) {
    if (full) {
        MEMSET(&ch8->mem, 0, sizeof(ch8->mem));
        const u8 CH8_FONTSET[SPRITES_SIZE] = {
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
        MEMCPY(CH8_ram, CH8_FONTSET, 0x50);
    } else {
        MEMSET(ch8->mem.stack, 0, sizeof(ch8->mem.stack));
        MEMSET(ch8->mem.ram + SPRITES_SIZE, 0, sizeof(ch8->mem.ram) - SPRITES_SIZE);
    }
    MEMSET(&ch8->reg, 0, sizeof(ch8->reg));
    MEMSET(&ch8->var, 0, sizeof(ch8->var));
    MEMSET(&ch8->input, 0, sizeof(ch8->input));
    MEMSET(&ch8->display, 0, sizeof(ch8->display));
    MEMSET(&ch8->set, 0, sizeof(ch8->set));
    CH8_width = CH8_DISPLAY_W;
    CH8_height = CH8_DISPLAY_H;
    CH8_size = CH8_DISPLAY_SIZE;
    CH8_PC = 0x200;
}

/// Instructions.
#define CLS() do { \
    MEMSET(CH8_pixels, 0, sizeof(CH8_pixels)); \
    ch8->draw.cb(&ch8->display, ch8->draw.user_data); \
} while(0)
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
#define SUB(a,b,r) do r = a - b; while(0)
#define SBC(a,b,r) do { CH8_V[0xF] = a > b; SUB(a,b,r); } while(0)
#define OR(a,b) do a |= b; while(0)
#define XOR(a,b) do a ^= b; while(0)
#define SHL(a) do { CH8_V[0xF] = (a >> 7) & 1; a <<= 1; } while(0)
#define SHR(a) do { CH8_V[0xF] = a & 1; a >>= 1; } while(0)
#define RND(a,mask) do a = __ch8_rand(ch8) & mask; while(0)
#define DRAW() do { \
    CH8_V[0xF] = 0; \
    for (u8 n = 0; n < CH8_n; n++) { \
    const u16 pos = (u16)(CH8_V[CH8_x] + ((CH8_V[CH8_y] + n) * CH8_width)); \
        for (u8 bit = 0; bit < 8; bit++) { \
            if (CH8_ram[CH8_I + n] & (0x80 >> bit)) { \
                u16 bitpos = pos + bit; \
                if (CH8_wrap) \
                    bitpos %= CH8_width; \
                CH8_V[0xF] |= CH8_pixels[bitpos]; \
                CH8_pixels[bitpos] ^= 1; \
            } \
        } \
    } \
    ch8->draw.cb(&ch8->display, ch8->draw.user_data); \
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
        case 0x5: SBC(CH8_V[CH8_x], CH8_V[CH8_y], CH8_V[CH8_x]); break; \
        case 0x6: SHR(CH8_V[CH8_x]); break; \
        case 0x7: SBC(CH8_V[CH8_y], CH8_V[CH8_x], CH8_V[CH8_x]); break; \
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
            LD(CH8_ram[CH8_I], (u8)(CH8_V[CH8_x] % 1000) / 100); \
            LD(CH8_ram[CH8_I + 1], (CH8_V[CH8_x] % 100) / 10); \
            LD(CH8_ram[CH8_I + 2], (CH8_V[CH8_x] % 10) / 1); \
            break; \
        case 0x55: \
            for (u8 i = 0; i <= CH8_x; i++) \
                LD(CH8_ram[CH8_I + i], CH8_V[i]); \
            break; \
        case 0x65: \
            for (u8 i = 0; i <= CH8_x; i++) \
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
bool ch8_savestate(const ch8_t *ch8, struct ch8_savestate *out_state) {
    if (!ch8 || !out_state) return false;
    out_state->seed = ch8->seed;
    MEMCPY(&out_state->reg, &ch8->reg, sizeof(out_state->reg));
    MEMCPY(out_state->stack, ch8->mem.stack, CH8_STACK_SIZE);
    MEMCPY(out_state->ram, ch8->mem.ram + SPRITES_SIZE, sizeof(out_state->ram));
    MEMCPY(out_state->display, ch8->display.pixels, sizeof(out_state->display));
    return true;
}

bool ch8_loadstate(ch8_t *ch8, const struct ch8_savestate *state) {
    if (!ch8 || !state) return false;
    ch8->seed = state->seed;
    MEMSET(&ch8->input, 0, sizeof(ch8->input));
    MEMCPY(&ch8->reg, &state->reg, sizeof(state->reg));
    MEMCPY(ch8->mem.stack, state->stack, CH8_STACK_SIZE);
    MEMCPY(ch8->mem.ram + SPRITES_SIZE, state->ram, sizeof(state->ram));
    MEMCPY(ch8->display.pixels, state->display, sizeof(state->display));
    return true;
}

void ch8_reset(ch8_t *ch8) {
    __ch8_reset(ch8, false);
}

bool ch8_init(ch8_t *ch8_out, ch8_cb_draw drw, void *drw_data, ch8_cb_sound snd, void *snd_data) {
    if (!ch8_out) return false;

    ch8_out->draw.cb = drw;
    ch8_out->draw.user_data = drw_data;
    ch8_out->sound.cb = snd;
    ch8_out->sound.user_data = snd_data;

    return true;
}

bool ch8_loadrom(ch8_t *ch8, const u8 *data, u32 len) {
    if (!ch8 || !data || !len) return false;
    if (len > CH8_MAX_ROM_SIZE) return false;
    __ch8_reset(ch8, true);
    MEMCPY(ch8->mem.ram + 0x200, data, len);
    return true;
}

void ch8_set_key(ch8_t *ch8, CH8_Key key, bool pressed) {
    CH8_keys[key] = pressed;
}

bool ch8_get_pixel(const struct Ch8_display *display, u8 x, u8 y) {
    return display->pixels[x + (display->width * y)];
}

void ch8_run(ch8_t *ch8) {
    for (u32 clock = 0; clock < 10; clock++) {
        FETCH();
        EXECUTE();
    }

    if (CH8_DT) {
        CH8_DT--;
    }

    if (CH8_ST) {
        CH8_ST--;
        ch8->sound.cb(ch8->draw.user_data);
    }
}
#endif /* CH8_IMPLEMENTATION */
#endif /* CH8_INCLUDE */