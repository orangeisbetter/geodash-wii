#include "screenManager.h"

#include <ogc/lwp_watchdog.h>

#include "../gfx.h"

ScreenManager screenManager;

static u64 startTick;

void changeScreen(gameState state, int fadeInTime, int fadeOutTime) {
    SYS_Report("the fade has started\n");
    screenManager.targetScreenState = state;
    screenManager.fadeOut = true;
    startTick = gettime();
}

void updateScreen() {
    if (!(screenManager.fadeIn || screenManager.fadeOut)) {
        return;
    }

    f32 timePassed = ticks_to_millisecs(gettime() - startTick) / 1000.0f;

    f32 w = view_width / 2.0f;
    f32 h = view_height / 2.0f;
    u8 alpha;

    if (screenManager.fadeOut) {
        alpha = (0.5f - timePassed) / 0.5f * 255;
    } else if (screenManager.fadeIn) {
        alpha = timePassed < 0.5f ? (timePassed - 0.5f) / 0.5f * 255 : 255;
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

    if (timePassed >= 0.5f) {
        if (screenManager.fadeOut) {
            screenManager.fadeOut = false;
            screenManager.fadeIn = true;
            screenManager.screenState = screenManager.targetScreenState;
        } else {
            screenManager.fadeIn = false;
            SYS_Report("the fade has completed\n");
        }
        startTick = gettime();
    }
}
