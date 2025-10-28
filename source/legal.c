#include "legal.h"

#include <stdlib.h>
#include <wiiuse/wpad.h>
#include <ogc/lwp_watchdog.h>

#include "gfx.h"

extern u16 view_width;
extern u16 view_height;

char* disclaimer =
    "This project is an unofficial fan-made recreation and requires legally obtained assets from the original game.\n"
    "No copyrighted assets are included. See `LEGAL.txt` for details.";

void legalNotice(BMFont* font, GXTexObj* fontTexture, s8* HWButton) {
    Mtx view, model, modelView;

    guMtxIdentity(view);

    f32 w = view_width / 2.0f;
    f32 h = view_height / 2.0f;

    bool exited = false;

    u64 startTick = gettime();
    u64 exitTick;

    while (*HWButton == -1) {
        WPAD_ScanPads();
        u32 pressed = WPAD_ButtonsDown(0);

        if (pressed & WPAD_BUTTON_HOME) {
            exit(0);
        }

        if (pressed & WPAD_BUTTON_A) {
            exitTick = gettime();
            exited = true;
        }

        f32 elapsed = ticks_to_millisecs(gettime() - startTick) / 1000.0f;
        f32 sinceExit = ticks_to_millisecs(gettime() - exitTick) / 1000.0f;

        guMtxIdentity(model);
        guMtxTransApply(model, model, 0.0f, 160.0f, 0.0f);
        guMtxConcat(view, model, modelView);
        f32 offset = Font_RenderText(font, fontTexture, "LEGAL NOTICE:", 32.0f, ALIGN_CENTER, 600.0f, modelView);

        guMtxTransApply(model, model, 0.0f, offset - 40.0f, 0.0f);
        guMtxConcat(view, model, modelView);
        offset = Font_RenderText(font, fontTexture, disclaimer, 26.0f, ALIGN_CENTER, 600.0f, modelView);

        guMtxTransApply(model, model, 0.0f, offset - 60.0f, 0.0f);
        guMtxConcat(view, model, modelView);
        Font_RenderText(font, fontTexture, "Press A to continue", 32.0f, ALIGN_CENTER, 600.0f, modelView);

        GX_ClearVtxDesc();
        GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
        GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);

        GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
        GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

        GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

        GX_SetNumChans(1);
        GX_SetNumTexGens(0);

        GFX_SetBlendMode(BLEND_ALPHA);

        guMtxIdentity(model);
        GX_LoadPosMtxImm(model, GX_PNMTX0);

        u8 alpha;

        if (elapsed < 0.5f) {
            alpha = (0.5f - elapsed) / 0.5f * 255;
        } else if (exited) {
            alpha = sinceExit < 0.5f ? (sinceExit - 0.5f) / 0.5f * 255 : 255;
        } else {
            alpha = 0;
        }

        u32 color = 0x00000000 + alpha;

        GX_Begin(GX_QUADS, GX_VTXFMT0, 4);

        GX_Position3f32(-w, h, 0.0f);
        GX_Color1u32(color);

        GX_Position3f32(w, h, 0.0f);
        GX_Color1u32(color);

        GX_Position3f32(w, -h, 0.0f);
        GX_Color1u32(color);

        GX_Position3f32(-w, -h, 0.0f);
        GX_Color1u32(color);

        GX_End();

        GFX_ResetDrawMode();

        GX_DrawDone();

        GFX_SwapBuffers();

        // Wait for next frame
        VIDEO_WaitVSync();

        if (exited) {
            f32 sinceExit = ticks_to_millisecs(gettime() - exitTick) / 1000.0f;

            if (sinceExit >= 0.5f) {
                return;
            }
        }
    }
}