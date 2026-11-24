#include "homescreen.h"

#include <stdio.h>
#include <wiiuse/wpad.h>
#include <mp3player.h>

#include "../render.h"
#include "../color.h"
#include "screenManager.h"
#include "gamelevel.h"
#include "menu.h"
#include "../pointer.h"

static f32 camX;
static f32 camY;
static f32 velX;

static const f32 defaultSpeed = 10.38f;

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

SpriteInfo geometryDash = {
    0, 100, 0, 1.0f, false, false
};
SpriteInfo playButton = {
    0, 0, 0, 1.0f, false, false
};
SpriteInfo characterSelectButton = {
    -110, 0, 0, 1.0f, false, false
};
SpriteInfo levelEditorButton = {
    110, 0, 0, 1.0f, false, false
};
SpriteInfo achievementsButton = {
    -60, -115, 1, 1.0f, false, false
};
SpriteInfo settingsButton = {
    0, -115, 1, 1.0f, false, false
};
SpriteInfo statsButton = {
    60, -115, 1, 1.0f, false, false
};

SpriteInfo* buttons[] = {&geometryDash, &playButton, &characterSelectButton, &levelEditorButton, &achievementsButton, &settingsButton, &statsButton};
texture_info* texs[7];
const f32 buttonTransitionTime = 0.1f;
f32 buttonTransition[7] = {0,0,0,0,0,0,0};

static void enterHomeScreen() {
    camX = 0.0f;
    camY = 120.0f;
    velX = defaultSpeed * 30.0f;
    ColorConfig colorConfig = {0};
    colorInit(colorConfig);
    {
        FILE* file = fopen("sd:/gdwii/assets/music/menuLoop.mp3", "rb");
        MP3Player_PlayFile(file, my_reader, NULL);
    }
    texs[0] = ht_search("GJ_logo_001.png");
    texs[1] = ht_search("GJ_playBtn_001.png");
    texs[2] = ht_search("GJ_garageBtn_001.png");
    texs[3] = ht_search("GJ_creatorBtn_001.png");
    texs[4] = ht_search("GJ_achBtn_001.png");
    texs[5] = ht_search("GJ_optionsBtn_001.png");
    texs[6] = ht_search("GJ_statsBtn_001.png");
}

static void exitHomeScreen() {
    loadMenuTextures();
    return;
}

static void runHomeScreen(f32 deltaTime) {
    u32 pressed = WPAD_ButtonsDown(0);

    const static f32 dt = 1.0f / 60.0f;

    for(int i=1; i<sizeof(buttons) / sizeof(buttons[0]); i++){
        if(PTR_IsOverSprite(texs[i], buttons[i], viewScale)){
            if(buttonTransition[i] >= buttonTransitionTime){
                buttonTransition[i] = buttonTransitionTime;
            }else{
                buttonTransition[i] += dt;
            }
            f32 scale = buttonTransition[i] / buttonTransitionTime * 0.1;
            buttons[i]->scale = 1.0f + scale;
            if (pressed & WPAD_BUTTON_A) {
                switch (i)
                {
                case 1:
                    changeState(&GAMESTATE_MENU, 0.5f);
                    break;
                default:
                    break;
                }
            }
        }else{
            if(buttonTransition[i] <= 0){
                buttonTransition[i] = 0;
            }else{
                buttonTransition[i] -= dt;
            }
            f32 scale = buttonTransition[i] / buttonTransitionTime * 0.1;
            buttons[i]->scale = 1.0f + scale;
            buttons[i]->scale = 1.0f + scale;
        }
    }
    
    camX += velX * dt;
}

static void renderHomeScreen() {
    GFX_ResetDrawMode();

    Mtx view;
    guMtxIdentity(view);
    
    RDR_drawBackground(&backgroundTexture, camX, camY, 0.45f, getColorChannel(1000).color);
    RDR_drawGround(&groundTexture, camX, camY, viewScale, getColorChannel(1001).color, false);
    // Draw line (VERY IMPORTANT)
    RDR_drawLine(view, camY, viewScale);
    GFX_ResetDrawMode();
    
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
    GFX_LoadTexture(&launchSheet, GX_TEXMAP6);
    
    guMtxIdentity(view);
    guMtxScaleApply(view, view, viewScale, viewScale, 1.0f);
    
    for(int i=0; i<sizeof(texs) / sizeof(texs[0]); i++){
        RDR_drawSpriteFromMap(texs[i], buttons[i], 1003, view);
    }
    
    PTR_Draw(viewScale);
}
