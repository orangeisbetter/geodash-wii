#include "gfx.h"

#include <gccore.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#define DEFAULT_FIFO_SIZE (256 * 1024)
#define CLIPPING_X (view_width / 2 + 64)
#define CLIPPING_Y (view_width / 2 + 64)

static void* frameBuffer[2];
static u32 fb = 0;
GXRModeObj* rmode;

static float px_width, px_height;
static u8* gp_fifo;

u16 view_width;
u16 view_height;
f32 aspect_ratio;
bool widescreen = false;
u8 refresh_rate;

static Mtx44 ortho;

void GFX_InitVideo() {
    VIDEO_Init();

    rmode = VIDEO_GetPreferredMode(NULL);

    if (CONF_GetAspectRatio() == CONF_ASPECT_16_9) {
        view_width = 852;
        view_height = 480;

        rmode->viWidth = 672;

        widescreen = true;
    } else {
        view_width = 640;
        view_height = 480;

        rmode->viWidth = 672;
    }

    aspect_ratio = CONF_GetAspectRatio() == CONF_ASPECT_16_9 ? 16.0f / 9.0f : 4.0f / 3.0f;

    if (rmode == &TVPal576IntDfScale || rmode == &TVPal576ProgScale) {
        rmode->viXOrigin = (VI_MAX_WIDTH_PAL - rmode->viWidth) / 2;
        rmode->viYOrigin = (VI_MAX_HEIGHT_PAL - rmode->viHeight) / 2;
    } else {
        rmode->viXOrigin = (VI_MAX_WIDTH_NTSC - rmode->viWidth) / 2;
        rmode->viYOrigin = (VI_MAX_HEIGHT_NTSC - rmode->viHeight) / 2;
    }

    px_width = rmode->fbWidth / (float)view_width;
    px_height = rmode->efbHeight / (float)view_height;

    VIDEO_SetBlack(true);
    VIDEO_Configure(rmode);
    VIDEO_Flush();
    VIDEO_WaitVSync();

    frameBuffer[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    frameBuffer[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    VIDEO_ClearFrameBuffer(rmode, frameBuffer[fb], COLOR_BLACK);

    VIDEO_SetNextFramebuffer(frameBuffer[fb]);
    VIDEO_SetBlack(true);
    VIDEO_Flush();
    VIDEO_WaitVSync();

    if (rmode->viTVMode & VI_NON_INTERLACE) {
        VIDEO_WaitVSync();
    }

    if (((rmode->viTVMode >> 2) & 0x7) == VI_PAL) {
        refresh_rate = 50;
    } else {
        refresh_rate = 60;
    }

    gp_fifo = memalign(32, DEFAULT_FIFO_SIZE);
}

void GFX_Init() {
    GX_AbortFrame();

    GXColor background = { 0x00, 0x00, 0x00, 0xFF };

    memset(gp_fifo, 0, DEFAULT_FIFO_SIZE);

    GX_Init(gp_fifo, DEFAULT_FIFO_SIZE);
    GX_SetCopyClear(background, 0x00ffffff);

    GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
    GX_SetDispCopyYScale((f32)rmode->xfbHeight / (f32)rmode->efbHeight);
    GX_SetScissor(0, 0, rmode->fbWidth, rmode->efbHeight);
    GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
    GX_SetDispCopyDst(rmode->fbWidth, rmode->xfbHeight);
    GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
    GX_SetFieldMode(rmode->field_rendering, ((rmode->viHeight == 2 * rmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));

    GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
    GX_SetCullMode(GX_CULL_NONE);

    GX_SetDispCopyGamma(GX_GM_1_0);

    guOrtho(ortho, view_height / 2, -(view_height / 2), -(view_width / 2), view_width / 2, 0.0f, 1000.0f);
    GX_LoadProjectionMtx(ortho, GX_ORTHOGRAPHIC);

    GX_InvVtxCache();
    GX_InvalidateTexAll();

    GFX_ResetDrawMode();

    GX_Flush();
    GX_DrawDone();

    VIDEO_SetBlack(false);
    VIDEO_Flush();
}

void GFX_SwapBuffers() {
    fb ^= 1;
    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetColorUpdate(GX_TRUE);
    GX_CopyDisp(frameBuffer[fb], GX_TRUE);

    VIDEO_SetNextFramebuffer(frameBuffer[fb]);
    VIDEO_Flush();
}