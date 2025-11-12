#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <fat.h>
#include <ogc/lwp_watchdog.h>
#include <asndlib.h>
#include <mp3player.h>
#include <cJSON.h>

#include "constants.h"
#include "BMFont.h"
#include "gfx.h"
#include "parse/level.h"
#include "parse/plist.h"
#include "util/hash_table.h"
#include "vendor/xml.h"
#include "legal.h"
#include "render.h"
#include "parse/object.h"
#include "color.h"
#include "game/gamelevel.h"
#include "game/homescreen.h"
#include "game/screenManager.h"

s8 HWButton = -1;

void wiiPowerPressed() {
    HWButton = SYS_POWEROFF;
}

void wiimotePowerPressed(s32 chan) {
    HWButton = SYS_POWEROFF;
}

void loadFont() {
    TPLFile fontFile;
    if (TPL_OpenTPLFromFile(&fontFile, ASSETS_PATH "bigFont-hd.tpl") != 1) {
        exit(0);
    }
    TPL_GetTexture(&fontFile, 0, &fontTexture);

    if (!parseBMFontFile(ASSETS_PATH "bigFont-hd.fnt", &font)) {
        exit(0);
    }
}

static const char* plistFiles[] = {
    ASSETS_PATH "GJ_GameSheet.plist",
    ASSETS_PATH "GJ_GameSheet02.plist",
    ASSETS_PATH "GJ_GameSheet03.plist",
    ASSETS_PATH "GJ_GameSheet04.plist",
    ASSETS_PATH "GJ_GameSheetIcons.plist",
    ASSETS_PATH "GJ_GameSheetGlow.plist"
};

u64 start = 0;

int main(int argc, char** argv) {
    GFX_InitVideo();
    GFX_Init();

    ASND_Init();
    MP3Player_Init();

    fatInitDefault();

    // Callbacks for poweroff
    // SYS_SetPowerCallback(wiiPowerPressed);
    // WPAD_SetPowerButtonCallback(wiimotePowerPressed);

    Mtx view;
    Mtx model;

    GX_SetCopyClear((GXColor){ 0x00, 0x00, 0x00, 0xff }, GX_MAX_Z24);

    // Set up camera and view matrix
    guVector cam = { 0.0f, 0.0f, 0.0f };
    guVector up = { 0.0f, 1.0f, 0.0f };
    guVector look = { 0.0f, 0.0f, -1.0f };
    guLookAt(view, &cam, &up, &look);

    loadFont();

    WPAD_Init();

    // Legal notice, blocking call
    legalNotice(&font, &fontTexture, &HWButton);

    TPLFile backgroundTPL;
    if (TPL_OpenTPLFromFile(&backgroundTPL, ASSETS_PATH "background.tpl") != 1) {
        exit(0);
    }

    TPL_GetTexture(&backgroundTPL, 0, &backgroundTexture);
    // TPL_CloseTPLFile(&backgroundTPL);

    // Loading screen
    for (int i = 0; i < 2; i++) {
        RDR_drawBackground(&backgroundTexture, 0.0, 120.0, 0.49f, 0x0066ffff);

        // Position loading text 40px left of right edge, 40px above bottom edge
        guMtxIdentity(model);
        guMtxTransApply(model, model, view_width / 2.0f - 40.0f, -(view_height / 2.0f) + 40.0f + 32.0f, 0.0f);
        Font_RenderText(&font, &fontTexture, "Loading...", 32.0f, ALIGN_RIGHT, 600.0f, model);

        GX_DrawDone();
        GFX_SwapBuffers();
    }

    VIDEO_WaitVSync();

    GFX_ResetDrawMode();
    GFX_SetBlendMode(BLEND_ALPHA);

    SYS_Report("Loading sprite sheets...\n");

    start = gettime();

    ht_init();
    openPlist(plistFiles, sizeof(plistFiles) / sizeof(plistFiles[0]));

    SYS_Report("Loaded sprite sheets. (%f ms)\n", ticks_to_microsecs(gettime() - start) / 1000.0f);

    // -----------------------------------------------------------------
    // Open objects file
    // -----------------------------------------------------------------

    start = gettime();

    objectDefaults = getObjectData(ASSETS_PATH "object.json");

    SYS_Report("Loaded object definitions. (%f ms)\n", ticks_to_microsecs(gettime() - start) / 1000.0f);

    // -----------------------------------------------------------------
    // Load textures
    // -----------------------------------------------------------------

    start = gettime();

    TPLFile groundTPL;
    if (TPL_OpenTPLFromFile(&groundTPL, ASSETS_PATH "ground.tpl") != 1) {
        exit(0);
    }

    TPL_GetTexture(&groundTPL, 0, &groundTexture);

    RDR_loadSpriteSheets();

    SYS_Report("Textures loaded. (%f ms)\n", ticks_to_microsecs(gettime() - start) / 1000.0f);

    // Set initial state to home (title) screen
    changeState(&GAMESTATE_HOME, -1.0f);

    // Main game loop
    while (HWButton == -1) {
        WPAD_ScanPads();
        u32 pressed = WPAD_ButtonsDown(0);

        if (pressed & WPAD_BUTTON_HOME) {
            exit(0);
        }

        runState(1.0f / 60.0f);

        GX_DrawDone();

        // Wait for next frame
        VIDEO_WaitVSync();
        GFX_SwapBuffers();
    }

    return 0;
}
