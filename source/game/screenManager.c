#include "screenManager.h"

#include <gccore.h>
#include <ogc/lwp_watchdog.h>

#include "../gfx.h"

static const GameState* currentState = NULL;
static const GameState* nextState;

static enum {
    FADE_IN,
    FADE_OUT,
    FADE_NONE,
} fadeStatus = FADE_NONE;

static f32 fadeTime = 0.0f;
static u64 startTick;

static void checkForTransition();
static void transition();
static void drawFade();

void changeState(const GameState* newState, f32 newFadeTime) {
    if (fadeStatus != FADE_NONE) {
        // Already in transition
        return;
    }
    nextState = newState;
    if (newFadeTime <= 0.0f) {
        // Instantaneous transition
        fadeStatus = FADE_NONE;
        transition();
    } else {
        fadeTime = newFadeTime;
        fadeStatus = FADE_OUT;
        startTick = gettime();
    }
}

void runState(f32 deltaTime) {
    if (fadeStatus != FADE_NONE) {
        checkForTransition();
    }

    if (currentState) {
        if (currentState->run) {
            currentState->run(deltaTime);
        }
        if (currentState->render) {
            currentState->render();
        }
    }

    if (fadeStatus != FADE_NONE) {
        drawFade();
    }
}

static void checkForTransition() {
    f32 timePassed = ticks_to_millisecs(gettime() - startTick) / 1000.0f;
    if (timePassed >= fadeTime) {
        if (fadeStatus == FADE_OUT) {
            fadeStatus = FADE_IN;
            transition();
        } else {
            fadeStatus = FADE_NONE;
        }
        startTick = gettime();
    }
}

static void transition() {
    if (currentState && currentState->exit)
        currentState->exit();
    currentState = nextState;
    if (currentState && currentState->enter)
        currentState->enter();
    SYS_Report("State changed to %p\n", currentState);
}

static void drawFade() {
    f32 timePassed = ticks_to_millisecs(gettime() - startTick) / 1000.0f;

    f32 w = view_width / 2.0f;
    f32 h = view_height / 2.0f;

    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);

    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

    GX_SetNumChans(1);
    GX_SetNumTexGens(0);

    GFX_SetBlendMode(BLEND_ALPHA);

    Mtx model;
    guMtxIdentity(model);
    GX_LoadPosMtxImm(model, GX_PNMTX0);

    u8 alpha;

    if (fadeStatus == FADE_OUT) {
        alpha = 0xff * timePassed / fadeTime;
    } else {
        alpha = 0xff * (1 - timePassed / fadeTime);
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
}

// void updateScreen() {
//     if (!(screenManager.fadeIn || screenManager.fadeOut)) {
//         return;
//     }

//     f32 timePassed = ticks_to_millisecs(gettime() - startTick) / 1000.0f;

//     f32 w = view_width / 2.0f;
//     f32 h = view_height / 2.0f;

//     GX_ClearVtxDesc();
//     GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
//     GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);

//     GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
//     GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

//     GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

//     GX_SetNumChans(1);
//     GX_SetNumTexGens(0);

//     GFX_SetBlendMode(BLEND_ALPHA);

//     Mtx model;
//     guMtxIdentity(model);
//     GX_LoadPosMtxImm(model, GX_PNMTX0);

//     u8 alpha;

//     if (screenManager.fadeIn) {
//         alpha = (0.5f - timePassed) / 0.5f * 255;
//     } else if (screenManager.fadeOut) {
//         alpha = timePassed < 0.5f ? (timePassed - 0.5f) / 0.5f * 255 : 255;
//     } else {
//         alpha = 0;
//     }

//     u32 color = 0x00000000 + alpha;

//     GX_Begin(GX_QUADS, GX_VTXFMT0, 4);

//     GX_Position3f32(-w, h, 0.0f);
//     GX_Color1u32(color);

//     GX_Position3f32(w, h, 0.0f);
//     GX_Color1u32(color);

//     GX_Position3f32(w, -h, 0.0f);
//     GX_Color1u32(color);

//     GX_Position3f32(-w, -h, 0.0f);
//     GX_Color1u32(color);

//     GX_End();

//     GFX_ResetDrawMode();
// }
