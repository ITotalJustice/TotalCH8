#include "ch8.h"


// TODO: remove these
/// Defaults.
#define CH8_VSYNC true
#define CH8_CLOCK 10

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


static inline void __ch8_memset(void *p, const int v, uint32_t len) {
    uint8_t *ptr = (uint8_t*)p;
    uint8_t value = (uint8_t)v;
    while (len--) *ptr++ = value;
}

static inline void __ch8_memcpy(void *dst, const void *src, uint32_t len) {
    uint8_t *d = (uint8_t*)dst;
    const uint8_t *s = (const uint8_t*)src;
    while (len--) *d++ = *s++;
}

#define MEMSET(p,v,len) __ch8_memset((void*)(p),(v),(len))
#define MEMCPY(d,s,len) __ch8_memcpy((void*)(d),(const void*)(s),(len))

static inline uint8_t __ch8_rand(struct CH8_Core* ch8) {
    ch8->seed = (214013*ch8->seed+2531011);
    return ((ch8->seed>>16)&0x7FFF) & 0xFF;
}

static void __ch8_reset(struct CH8_Core* ch8, const bool full) {
    if (full) {
        MEMSET(&ch8->mem, 0, sizeof(ch8->mem));
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
        MEMCPY(CH8_ram, CH8_FONTSET, 0x50);
    } else {
        MEMSET(ch8->mem.stack, 0, sizeof(ch8->mem.stack));
        MEMSET(ch8->mem.ram + SPRITES_SIZE, 0, sizeof(ch8->mem.ram) - SPRITES_SIZE);
    }
    MEMSET(&ch8->reg, 0, sizeof(ch8->reg));
    MEMSET(&ch8->var, 0, sizeof(ch8->var));
    MEMSET(&ch8->input, 0, sizeof(ch8->input));
    MEMSET(&ch8->display, 0, sizeof(ch8->display));
    CH8_width = CH8_DISPLAY_W;
    CH8_height = CH8_DISPLAY_H;
    CH8_size = CH8_DISPLAY_SIZE;
    CH8_PC = 0x200;
}

/// Instructions.
#define CLS() do { \
    MEMSET(CH8_pixels, 0, sizeof(CH8_pixels)); \
    ch8->display.draw = true; \
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
    for (uint8_t n = 0; n < CH8_n; n++) { \
    const uint16_t pos = (uint32_t)(CH8_V[CH8_x] + ((CH8_V[CH8_y] + n) * CH8_width)); \
        for (uint8_t bit = 0; bit < 8; bit++) { \
            if (CH8_ram[CH8_I + n] & (0x80 >> bit)) { \
                uint16_t bitpos = pos + bit; \
                if (bitpos >= CH8_DISPLAY_SIZE) { \
                	if (CH8_wrap) { \
                		bitpos %= CH8_width; \
                	} \
                	else { \
                		continue; \
                	} \
                } \
                CH8_V[0xF] |= CH8_pixels[bitpos]; \
                CH8_pixels[bitpos] ^= 1; \
            } \
        } \
    } \
    ch8->display.draw = true; \
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
            LD(CH8_ram[CH8_I], (uint8_t)(CH8_V[CH8_x] % 1000) / 100); \
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
bool ch8_savestate(const struct CH8_Core* ch8, struct ch8_savestate* out_state) {
    if (!ch8 || !out_state) return false;
    out_state->seed = ch8->seed;
    MEMCPY(&out_state->reg, &ch8->reg, sizeof(out_state->reg));
    MEMCPY(out_state->stack, ch8->mem.stack, CH8_STACK_SIZE);
    MEMCPY(out_state->ram, ch8->mem.ram + SPRITES_SIZE, sizeof(out_state->ram));
    MEMCPY(out_state->display, ch8->display.pixels, sizeof(out_state->display));
    return true;
}

bool ch8_loadstate(struct CH8_Core* ch8, const struct ch8_savestate* state) {
    if (!ch8 || !state) return false;
    ch8->seed = state->seed;
    MEMSET(&ch8->input, 0, sizeof(ch8->input));
    MEMCPY(&ch8->reg, &state->reg, sizeof(state->reg));
    MEMCPY(ch8->mem.stack, state->stack, CH8_STACK_SIZE);
    MEMCPY(ch8->mem.ram + SPRITES_SIZE, state->ram, sizeof(state->ram));
    MEMCPY(ch8->display.pixels, state->display, sizeof(state->display));
    return true;
}

void ch8_reset(struct CH8_Core* ch8) {
    __ch8_reset(ch8, false);
}

bool ch8_init(struct CH8_Core* ch8_out) {
    if (!ch8_out) return false;
    MEMSET(ch8_out, 0, sizeof(struct CH8_Core));
    ch8_out->vsync = CH8_VSYNC;
    ch8_out->clock = CH8_CLOCK;
    return true;
}

bool ch8_loadrom(struct CH8_Core* ch8, const uint8_t* data, uint32_t len) {
    if (!ch8 || !data || !len) return false;
    if (len > CH8_MAX_ROM_SIZE) return false;
    __ch8_reset(ch8, true);
    MEMCPY(ch8->mem.ram + 0x200, data, len);
    return true;
}

void ch8_set_key(struct CH8_Core* ch8, CH8_Key key, bool pressed) {
    CH8_keys[key] = pressed;
}

bool ch8_should_draw(const struct CH8_Core* ch8) {
    return ch8->display.draw;
}

bool ch8_should_sound(const struct CH8_Core* ch8) {
    return ch8->reg.st > 0;
}

uint32_t ch8_get_clock(struct CH8_Core* ch8) {
    return ch8->clock;
}

void ch8_set_clock(struct CH8_Core* ch8, uint32_t clock) {
    ch8->clock = clock;
}

void ch8_toggle_vsync(struct CH8_Core* ch8, bool enable) {
    ch8->vsync = enable;
}

bool ch8_get_pixel(const struct CH8_Core* ch8, uint8_t x, uint8_t y) {
    return ch8->display.pixels[x + (ch8->display.width * y)];
}

void ch8_run(struct CH8_Core* ch8) {
    ch8->display.draw = false;

    for (uint32_t clock = 0; clock < ch8->clock; clock++) {
        FETCH();
        EXECUTE();
        // break early if a draw call is made
        if (ch8->display.draw && ch8->vsync) {
            break;
        }
    }

    if (CH8_DT) {
        CH8_DT--;
    }

    if (CH8_ST) {
        CH8_ST--;
    }
}
