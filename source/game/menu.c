#include "menu.h"

#include <wiiuse/wpad.h>
#include <mp3player.h>

#include "../constants.h"
#include "../render.h"
#include "../color.h"
#include "../gfx.h"
#include "screenManager.h"
#include "gamelevel.h"
#include "../parse/levelinfo.h"

typedef struct {
    u32 startColor;
    u32 endColor;
} MenuColors;

static TPLFile squareTPL;
static GXTexObj square02;

static f32 camX;
static f32 camY;
static f32 timePassed;
static MenuColors backgroundColor;
static int levelIndex = 0;

static const f32 viewScale = 1.5f;
static const int numLevels = sizeof(mainLevels) / sizeof(mainLevels[0]);

static void menuEnter();
static void menuExit();
static void menuRun(f32 deltaTime);
static void menuRender();

static const MenuColors menuColors[] = {
    { 0x0000FC00, 0x00008900 },
    { 0xFC00FC00, 0x89008900 },
    { 0xFC007C00, 0x89004300 },
    { 0xFC000000, 0x89000000 },
    { 0xFC7C0000, 0x89430000 },
    { 0xFCFC0000, 0x89890000 },
    { 0x00FC0000, 0x00890000 },
    { 0x00FCFC00, 0x00898900 },
    { 0x007CFC00, 0x00438900 }
};

const GameState GAMESTATE_MENU = {
    .enter = menuEnter,
    .exit = menuExit,
    .run = menuRun,
    .render = menuRender
};

static s32 my_reader(void* cb_data, void* dst, s32 len) {
    FILE* f = (FILE*)cb_data;
    return fread(dst, 1, len, f);
}

void loadMenuTextures() {
    if (TPL_OpenTPLFromFile(&squareTPL, ASSETS_PATH "squares.tpl") != 1) {
        // SYS_Report("ERROROROROROROROR\n");
    } else {
        TPL_GetTexture(&squareTPL, 0, &square02);
        // SYS_Report("must have worked\n");
    }
}

static void menuEnter() {
    camX = 0.0f;
    camY = 120.0f;
    backgroundColor = menuColors[0];
    // levelIndex = 0;
    timePassed = 0.0f;
    {
        FILE* file = fopen("sd:/gdwii/assets/music/menuLoop.mp3", "rb");
        MP3Player_PlayFile(file, my_reader, NULL);
    }
}

static void menuExit() {
}

static void menuRun(f32 deltaTime) {
    u32 pressed = WPAD_ButtonsDown(0);

    if (pressed & WPAD_BUTTON_LEFT) {
        levelIndex--;
        if (levelIndex < 0) {
            levelIndex = numLevels;
        }
    }
    if (pressed & WPAD_BUTTON_RIGHT) {
        levelIndex++;
        if (levelIndex == numLevels + 1) {
            levelIndex = 0;
        }
    }

    backgroundColor = menuColors[levelIndex % (sizeof(menuColors) / sizeof(menuColors[0]))];

    if (pressed & WPAD_BUTTON_A) {
        LevelInfo* levelInfo = getLevelInfoById(mainLevels[levelIndex]);
        if (levelInfo != NULL) {
            changeState(&GAMESTATE_LEVEL, .5);
            loadLevel(mainLevels[levelIndex]);
        }
    }
}

static void menuRender() {
    GFX_ResetDrawMode();

    Mtx view;
    guMtxIdentity(view);

    RDR_drawMenuBackground(camX, camY, 0.49f, backgroundColor.startColor, backgroundColor.endColor);
    RDR_drawGround(&groundTexture, camX, camY, viewScale, backgroundColor.startColor);
    RDR_drawLine(view, camY, viewScale);

    GFX_ResetDrawMode();

    LevelInfo* levelInfo = levelIndex == numLevels ? NULL : getLevelInfoById(mainLevels[levelIndex]);

    // Set up camera view matrix
    guMtxTrans(view, 0, 0, -10.0f);
    guMtxScaleApply(view, view, viewScale, viewScale, 1.0f);

    // Prepare for object rendering
    GFX_SetBlendMode(BLEND_ALPHA);
    GFX_LoadTexture(&spriteSheet, GX_TEXMAP0);
    GFX_LoadTexture(&gameSheet02, GX_TEXMAP1);
    GFX_LoadTexture(&gameSheet03, GX_TEXMAP2);
    GFX_LoadTexture(&gameSheet04, GX_TEXMAP3);
    GFX_LoadTexture(&gameSheetIcons, GX_TEXMAP4);
    GFX_LoadTexture(&gameSheetGlow, GX_TEXMAP5);

    guMtxIdentity(view);
    guMtxScaleApply(view, view, viewScale, viewScale, 1.0f);

    // corners
    texture_info* corner = ht_search("GJ_sideArt_001.png");
    f32 cornerXPos = ((view_width / 2) / viewScale) - (corner->spriteSize.x / 2);
    f32 cornerYPos = ((view_height / 2) / viewScale) - (corner->spriteSize.y / 2);
    RDR_drawSpriteFromMap(corner, (SpriteInfo){ -cornerXPos, -cornerYPos, 0, false, false }, 1003, view);
    RDR_drawSpriteFromMap(corner, (SpriteInfo){ cornerXPos, -cornerYPos, 0, true, false }, 1003, view);

    // top bar
    texture_info* topBar = ht_search("GJ_topBar_001.png");
    f32 topBarYPos = ((view_height / 2) / viewScale) - (topBar->spriteSize.y / 2);
    RDR_drawSpriteFromMap(topBar, (SpriteInfo){ 0, topBarYPos, 0, false, false }, 1003, view);

    // back arrow
    texture_info* backArrow = ht_search("GJ_arrow_01_001.png");
    f32 backButtonXPos = ((view_width / 2) / viewScale) - (backArrow->spriteSize.x / 2) - 10;
    f32 backButtonYPos = ((view_height / 2) / viewScale) - (backArrow->spriteSize.y / 2) - 5;
    RDR_drawSpriteFromMap(backArrow, (SpriteInfo){ -backButtonXPos, backButtonYPos, 0, false, false }, 1003, view);

    // arrows
    texture_info* arrow = ht_search("navArrowBtn_001.png");
    f32 arrowXPos = ((view_width / 2) / viewScale) - (arrow->spriteSize.x / 2) - 10;
    f32 arrowYPos = 0;
    RDR_drawSpriteFromMap(arrow, (SpriteInfo){ -arrowXPos, arrowYPos, 0, true, false }, 1003, view);
    RDR_drawSpriteFromMap(arrow, (SpriteInfo){ arrowXPos, arrowYPos, 0, false, false }, 1003, view);

    // circle
    const int circleSpacing = 20;
    texture_info* circle = ht_search("d_link_b_01_color_001.png");
    f32 circleXPos = ((numLevels)*circleSpacing) / 2.0f;
    f32 circleYPos = ((view_height / 2) / viewScale) - (circle->spriteSize.y / 2) - 16;
    for (int i = 0; i < numLevels + 1; i++) {
        u32 color = 0x7f7f7fff;
        if (i == levelIndex) {
            color = 0xffffffff;
        }
        RDR_drawSpriteFromMap2(circle, (SpriteInfo){ -circleXPos + (circleSpacing * i), -circleYPos, 0, false, false }, color, false, view);
    }

    if (levelInfo != NULL) {
        // gray background
        RDR_drawRect(&square02, 0, 90, 516, 146, 0x0000007f);
        RDR_drawRect(&square02, 0, -45, 516, 36, 0x0000007f);
        RDR_drawRect(&square02, 0, -118, 516, 36, 0x0000007f);

        // text
        const f32 titleFontSize = 30;
        Mtx model, modelView;
        guMtxTrans(model, 0.0f, 2.0f, 0.0f);
        guMtxConcat(view, model, modelView);
        Font_RenderText(&font, &fontTexture, "Normal Mode", 18.0f, ALIGN_CENTER, 300, modelView);
        guMtxTrans(model, 0.0f, -48.0f, 0.0f);
        guMtxConcat(view, model, modelView);
        Font_RenderText(&font, &fontTexture, "Practice Mode", 18.0f, ALIGN_CENTER, 300, modelView);
        guMtxTrans(model, 0.0f, 90 - (titleFontSize / 2), 0.0f);
        guMtxConcat(view, model, modelView);
        Font_RenderText(&font, &fontTexture, levelInfo->name, titleFontSize, ALIGN_CENTER, 516, modelView);
    } else if (levelIndex == numLevels) {
        Mtx model, modelView;
        guMtxTrans(model, 0.0f, 48.0f, 0.0f);
        guMtxConcat(view, model, modelView);
        Font_RenderText(&font, &fontTexture, "Coming Soon!", 32.0f, ALIGN_CENTER, view_width, modelView);
    } else {
        Mtx model, modelView;
        guMtxTrans(model, 0.0f, 16.0f, 0.0f);
        guMtxConcat(view, model, modelView);
        Font_RenderText(&font, &fontTexture, "Invalid level ID", 32.0f, ALIGN_CENTER, view_width, modelView);
    }

    GFX_ResetDrawMode();
}