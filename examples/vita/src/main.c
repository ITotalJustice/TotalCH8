#define CH8_IMPLEMENTATION
#include "../../../ch8.h"
#define CH8_ROMS_IMPLEMENTATION
#include "../../../ch8_ext_roms.h"

#include <psp2/types.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/display.h>
#include <psp2/gxm.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/ctrl.h>
#include <string.h>
#include <stdbool.h>

// based on https://github.com/vitasdk/samples/tree/master/ime

#define SCALE 15

#define ARGB(a,r,g,b) ((a << 24) | (r << 16) | (g << 8) | b)
#define RGB(r,g,b) ARGB(0xFF,r,g,b)

#define ALIGN(x, a)	(((x) + ((a) - 1)) & ~((a) - 1))
#define DISPLAY_WIDTH			960
#define DISPLAY_HEIGHT			544
#define DISPLAY_STRIDE_IN_PIXELS	1024
#define DISPLAY_BUFFER_COUNT		2
#define DISPLAY_MAX_PENDING_SWAPS	1

struct DisplayBuffer {
	unsigned *fb;
	SceGxmSyncObject*sync;
	SceGxmColorSurface surf;
	SceUID uid;
};

static unsigned backBufferIndex = 0;
static unsigned frontBufferIndex = 1;
static struct DisplayBuffer dbuf[DISPLAY_BUFFER_COUNT];

static void *dram_alloc(unsigned size, SceUID *uid){
	void *mem;
	*uid = sceKernelAllocMemBlock("gpu_mem", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, ALIGN(size, 256 * 1024), NULL);
	sceKernelGetMemBlockBase(*uid, &mem);
	sceGxmMapMemory(mem, ALIGN(size,256*1024), SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE);
	return mem;
}

static void gxm_vsync_cb(const void *callback_data){
	sceDisplaySetFrameBuf(&(SceDisplayFrameBuf){sizeof(SceDisplayFrameBuf),
		*((void **)callback_data), DISPLAY_STRIDE_IN_PIXELS, 0,
		DISPLAY_WIDTH, DISPLAY_HEIGHT}, SCE_DISPLAY_SETBUF_NEXTFRAME);
}

static void gxm_init() {
	sceGxmInitialize(&(SceGxmInitializeParams){ 0, DISPLAY_MAX_PENDING_SWAPS,
		gxm_vsync_cb, sizeof(void *), SCE_GXM_DEFAULT_PARAMETER_BUFFER_SIZE });
	for (unsigned i = 0; i < DISPLAY_BUFFER_COUNT; i++) {
		dbuf[i].fb = dram_alloc(4 * DISPLAY_STRIDE_IN_PIXELS * DISPLAY_HEIGHT, &dbuf[i].uid);
		sceGxmColorSurfaceInit(&dbuf[i].surf, SCE_GXM_COLOR_FORMAT_U8U8U8U8_RGBA,
			SCE_GXM_COLOR_SURFACE_LINEAR, SCE_GXM_COLOR_SURFACE_SCALE_NONE,
			SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT, DISPLAY_WIDTH, DISPLAY_HEIGHT,
			DISPLAY_STRIDE_IN_PIXELS, dbuf[i].fb);
		sceGxmSyncObjectCreate(&dbuf[i].sync);
	}
}

static void gxm_swap() {
	sceGxmDisplayQueueAddEntry(dbuf[frontBufferIndex].sync, dbuf[backBufferIndex].sync, &dbuf[backBufferIndex].fb);
	frontBufferIndex ^= 1;
	backBufferIndex ^= 1;
}

static void gxm_term() {
	sceGxmTerminate();
	for (int i = 0; i < DISPLAY_BUFFER_COUNT; i++) {
		sceKernelFreeMemBlock(dbuf[i].uid);
	}
}

static void inline draw_pixel(unsigned *fb, int x, int y, unsigned col) {
	const int pos = (y * DISPLAY_STRIDE_IN_PIXELS) + x;
	fb[pos] = col;
}

static void inline draw_rect(unsigned *fb, int x, int y, int w, int h, unsigned col) {
	for (int _y = 0; _y < h; _y++) {
        for (int _x = 0; _x < w; _x++) {
            draw_pixel(fb, _x + x, _y + y, col);
        }  
    }
}

int main(int argc, const char **argv) {
	ch8_t ch8 = {0};
    ch8_init(&ch8);
    ch8_loadbuiltinrom(&ch8, INVADERS);

	gxm_init();
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG); // TODO: control

	while (1) {
		SceCtrlData ctrl = {0};
		sceCtrlPeekBufferPositive(0, &ctrl, 1);

		ch8_run(&ch8);

		memset(dbuf[backBufferIndex].fb, 0, DISPLAY_HEIGHT * DISPLAY_STRIDE_IN_PIXELS * 4);

		for (uint8_t y = 0; y < CH8_DISPLAY_H; y++) {
            for (uint8_t x = 0; x < CH8_DISPLAY_W; x++) {
                if (ch8_get_pixel(&ch8,x,y)) {
                	draw_rect(dbuf[backBufferIndex].fb, x * SCALE, y * SCALE, SCALE, SCALE, RGB(0,0xFF,0));
				}
            }
        }

		gxm_swap();
		sceDisplayWaitVblankStart();
	}

	gxm_term();
	sceKernelExitProcess(0);

	return 0;
}