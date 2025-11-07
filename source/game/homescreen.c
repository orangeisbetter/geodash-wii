#include "homescreen.h"

#include <stdio.h>
#include <wiiuse/wpad.h>
#include <mp3player.h>

#include "../render.h"
#include "../color.h"
#include "screenManager.h"
#include "gamelevel.h"
#include "menu.h"

static f32 camX;
static f32 camY;
static f32 velX;

static const f32 defaultSpeed = 5.0f;

static const f32 viewScale = 1.5f;

static void enterHomeScreen();
static void exitHomeScreen();
static void runHomeScreen(f32 deltaTime);
static void renderHomeScreen();

const GameState GAMESTATE_HOME = {
    .enter = enterHomeScreen,
    .exit = exitHomeScreen,
    .run = runHomeScreen,
    .render = renderHomeScreen
};

static s32 my_reader(void* cb_data, void* dst, s32 len) {
    FILE* f = (FILE*)cb_data;
    return fread(dst, 1, len, f);
}

static void enterHomeScreen() {
    camX = 0.0f;
    camY = 120.0f;
    velX = defaultSpeed * 30.0f;

    colorInit();
    {
        FILE* file = fopen("sd:/gdwii/assets/music/menuLoop.mp3", "rb");
        MP3Player_PlayFile(file, my_reader, NULL);
    }
}

static void exitHomeScreen() {
    loadMenuTextures();
    return;
}

static void runHomeScreen(f32 deltaTime) {
    u32 pressed = WPAD_ButtonsDown(0);

    if (pressed & WPAD_BUTTON_A) {
        // loadLevel(1);
        changeState(&GAMESTATE_MENU, 0.5f);
    }
    const static f32 dt = 1.0f / 60.0f;

    camX += velX * dt;
}

static void renderHomeScreen() {
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
