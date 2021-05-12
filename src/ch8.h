#ifndef _CH8_INCLUDE_H_
#define _CH8_INCLUDE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#if defined _WIN32 || defined __CYGWIN__
  #ifdef BUILDING_LIB
    #define CH8_PUBLIC __declspec(dllexport)
  #else
    #define CH8_PUBLIC __declspec(dllimport)
  #endif
#else
  #ifdef BUILDING_LIB
      #define CH8_PUBLIC __attribute__ ((visibility ("default")))
  #else
      #define CH8_PUBLIC
  #endif
#endif

/// API
#define CH8_VERSION_MAJOR 1
#define CH8_VERSION_MINOR 0
#define CH8_VERSION_MACRO 0
#define CH8_VERSION (CH8_VERSION_MAJOR << 24 | CH8_VERSION_MINOR << 16 | CH8_VERSION_MACRO << 8)

/// Sizes.
enum {
    SPRITE_SIZE = 0x5,
    SPRITES_SIZE = 0x50,
    CH8_VREG_SIZE = 0x10,
    CH8_RESERVED = 0x200,
    CH8_RAM_SIZE = 0x1000,
    CH8_STACK_SIZE = 0x10,
    CH8_MAX_ROM_SIZE = CH8_RAM_SIZE - 0x200,
};

/// Chip8 specific.
enum {
    CH8_DISPLAY_W = 64,
    CH8_DISPLAY_H = 32,
    CH8_DISPLAY_SIZE = (CH8_DISPLAY_W * CH8_DISPLAY_H),
};

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
    uint16_t i;   /// index.
    uint16_t pc;  /// program counter
    uint16_t sp;  /// stack pointer.
};

struct Ch8_mem {
    uint8_t ram[CH8_RAM_SIZE];
    uint16_t stack[CH8_STACK_SIZE];
};

struct Ch8_display {
    bool draw;
    bool wrap;
    uint8_t pixels[CH8_DISPLAY_SIZE];
    uint8_t width, height;
    uint16_t size;
};

struct Ch8_input {
    bool keys[0x10];
};

struct Ch8_var {
    uint8_t n;
    uint8_t x;
    uint8_t y;
    uint8_t kk;
    uint16_t nnn;
    uint16_t opcode;
};

struct ch8_savestate {
    uint32_t version;
    uint32_t seed;
    struct Ch8_reg reg;
    uint8_t ram[CH8_RESERVED-SPRITES_SIZE];
    uint16_t stack[CH8_STACK_SIZE];
    uint8_t display[CH8_DISPLAY_SIZE];
};

struct CH8_Core {
    struct Ch8_reg reg;
    struct Ch8_mem mem;
    struct Ch8_display display;
    struct Ch8_input input;
    struct Ch8_var var;
    uint32_t seed;
    uint32_t clock;
    bool vsync;
};

/// API CALLS
void CH8_PUBLIC ch8_set_key(struct CH8_Core* ch8, CH8_Key key, bool pressed);
bool CH8_PUBLIC ch8_loadrom(struct CH8_Core* ch8, const uint8_t* data, uint32_t len);
bool CH8_PUBLIC ch8_savestate(const struct CH8_Core* ch8, struct ch8_savestate* out_state);
bool CH8_PUBLIC ch8_loadstate(struct CH8_Core* ch8, const struct ch8_savestate* state);
void CH8_PUBLIC ch8_reset(struct CH8_Core* ch8);
bool CH8_PUBLIC ch8_should_draw(const struct CH8_Core* ch8);
bool CH8_PUBLIC ch8_should_sound(const struct CH8_Core* ch8);
uint32_t CH8_PUBLIC ch8_get_clock(struct CH8_Core* ch8);
void CH8_PUBLIC ch8_set_clock(struct CH8_Core* ch8, uint32_t clock);
void CH8_PUBLIC ch8_toggle_vsync(struct CH8_Core* ch8, bool enable);
bool CH8_PUBLIC ch8_get_pixel(const struct CH8_Core* ch8, uint8_t x, uint8_t y);
void CH8_PUBLIC ch8_run(struct CH8_Core* ch8);
bool CH8_PUBLIC ch8_init(struct CH8_Core* ch8_out);

#ifdef __cplusplus
}
#endif

#endif /* _CH8_INCLUDE_H_ */
