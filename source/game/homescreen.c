#include "homescreen.h"

#include <stdio.h>
#include <mp3player.h>

#include "../render.h"
#include "../color.h"

static f32 camX;
static f32 camY;
static f32 velX;
const static f32 defaultSpeed = 10.386f;

static const f32 viewScale = 1.5f;

static s32 my_reader(void* cb_data, void* dst, s32 len) {
    FILE* f = (FILE*)cb_data;
    return fread(dst, 1, len, f);
}

void loadHomeScreen() {
    camX = 0.0f;
    camY = 120.0f;
    velX = defaultSpeed * 30.0f;

    colorInit();
    {
        FILE* file = fopen("sd:/gdwii/assets/music/menuLoop.mp3", "rb");
        MP3Player_PlayFile(file, my_reader, NULL);
    }
}

void runHomeScreen() {
    const static f32 dt = 1.0f / 60.0f;

    camX += velX * dt;

    GFX_ResetDrawMode();

    Mtx view;
    guMtxIdentity(view);

    RDR_drawBackground(&backgroundTexture, camX, camY, 0.49f, getColorChannel(1000).color);
    RDR_drawGround(&groundTexture, camX, camY, viewScale, getColorChannel(1001).color);

    // Set up camera view matrix
    guMtxTrans(view, -camX, -camY, -10.0f);
    guMtxScaleApply(view, view, viewScale, viewScale, 1.0f);

    // Prepare for object rendering
    GFX_SetBlendMode(BLEND_ALPHA);
    GFX_LoadTexture(&spriteSheet, GX_TEXMAP0);
    GFX_LoadTexture(&gameSheet02, GX_TEXMAP1);
    GFX_LoadTexture(&gameSheet03, GX_TEXMAP2);
    GFX_LoadTexture(&gameSheet04, GX_TEXMAP3);
    GFX_LoadTexture(&gameSheetIcons, GX_TEXMAP4);
    GFX_LoadTexture(&gameSheetGlow, GX_TEXMAP5);

    texture_info* tex = ht_search("GJ_playBtn_001.png");

    guMtxIdentity(view);
    guMtxScaleApply(view, view, viewScale, viewScale, 1.0f);

    RDR_drawSpriteFromMap(tex, (SpriteInfo){ 0, 0, 0, false, false }, 1003, view);

    // Draw line (VERY IMPORTANT)
    RDR_drawLine(view, camY, viewScale);

    GFX_ResetDrawMode();
}