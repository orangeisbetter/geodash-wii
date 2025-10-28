#include "gamelevel.h"

#include <gccore.h>
#include <ogc/lwp_watchdog.h>
#include <stdio.h>
#include <stdlib.h>
#include <mp3player.h>
#include <wiiuse/wpad.h>

#include "../constants.h"
#include "../parse/level.h"
#include "../render.h"
#include "../color.h"
#include "../gfx.h"

ObjectData* objectDefaults = NULL;

static s32 my_reader(void* cb_data, void* dst, s32 len) {
    FILE* f = (FILE*)cb_data;
    return fread(dst, 1, len, f);
}

static int currentLevelId;

static f32 camX;
static f32 camY;
static f32 velX;
static f32 velY;

static f32 gravity;
const static f32 defaultSpeed = 10.386f;

static SpriteInfo player = {
    0,
    15,
    0,
    false,
    false,
};

static LevelObject* objects;
static int objectsSize;

static RenderCache renderCache = { 0 };

static const f32 viewScale = 1.5f;

static u64 lastFrameCPU = 0;
static u64 lastFrameRender = 0;
static u64 lastFrameTotal = 0;

static const char* levelNames[] = {
    "Stereo Madness",
    "Back On Track",
    "Polargeist",
    "Dry Out",
    "Base After Base",
    "Can't Let Go",
    "Jumper",
    "Time Machine",
    "Cycles",
    "xStep"
};

static const char* levelMusic[] = {
    "StereoMadness.mp3",
    "BackOnTrack.mp3",
    "Polargeist.mp3",
    "DryOut.mp3",
    "BaseAfterBase.mp3",
    "CantLetGo.mp3",
    "Jumper.mp3",
    "TimeMachine.mp3",
    "Cycles.mp3",
    "xStep.mp3"
};

// static int renderCacheCapacity;
// static int renderCacheSize;
// static RenderObject* renderCache;

static int compare(const void* a, const void* b) {
    int first = ((RenderObject*)a)->z_layer - ((RenderObject*)b)->z_layer;
    if (first != 0) {
        return first;
    }
    return ((RenderObject*)a)->z_order - ((RenderObject*)b)->z_order;
}

int loadLevel(int levelId) {
    // -----------------------------------------------------------------
    // Load level
    // -----------------------------------------------------------------

    currentLevelId = levelId;

    u64 start = gettime();

    char levelPath[64];
    snprintf(levelPath, 64, ASSETS_PATH "levels/%d.txt", levelId + 1);

    char* levelString = GDL_GetLevelString(levelPath);
    if (levelString == NULL) {
        return 0;
    }

    objects = GDL_ParseLevelString(levelString, &objectsSize, objectDefaults);

    free(levelString);

    SYS_Report("Level loaded: %d objects. (%f ms)\n", objectsSize, ticks_to_microsecs(gettime() - start) / 1000.0f);
    camX = 0.0f;
    camY = 120.0f;

    velX = defaultSpeed * 30.0f;
    velY = 0.0f;

    gravity = -(10.0f / 12.0f) * defaultSpeed * defaultSpeed * 30.0f;

    colorInit();

    if (!RDR_RenderCacheInit(&renderCache, 32)) {
        SYS_Report("Unable to allocate memory for object cache\n");
        return 0;
    }

    SYS_Report("Render cache created with size %d and capacity %d, p = %p\n", renderCache.size, renderCache.capacity, renderCache.objects);

    {
        char buf[64];
        snprintf(buf, 64, ASSETS_PATH "music/%s", levelMusic[levelId]);
        FILE* file = fopen(buf, "rb");
        MP3Player_PlayFile(file, my_reader, NULL);
    }

    // u64 jumpTime = 30000000;
    // u64 jumpStartTime = 0;

    return 1;
}

void runLevel(Mtx view, Mtx modelView, u32 pressed) {
    // while (HWButton == -1) {
    //  if (gettime() - jumpStartTime < jumpTime) {
    //      // SYS_Report("Time left: %u\n", gettime() - jumpStartTime);
    //      u64 dy = (gettime() - jumpStartTime) / 600000;
    //      u64 newY = dy;
    //      if ((gettime() - jumpStartTime) < jumpTime / 2) {
    //      } else {
    //          newY = (jumpTime - (gettime() - jumpStartTime)) / 600000;
    //      }
    //      player.y = newY + 15;
    //      // player.rotation = jumpTime / (gettime() - jumpStartTime) * 90;
    //  }

    // if (pressed & WPAD_BUTTON_A) {
    //     SYS_Report("Player jumped!!! at time: %u\n", gettime());
    //     jumpStartTime = gettime();
    // }

    // --------------------------------------------------------------------
    // Game logic
    // --------------------------------------------------------------------

    const static f32 dt = 1.0f / 60.0f;

    u64 allStart = gettime();

    f32 lastX = camX;

    velY += gravity * dt;

    if (pressed & WPAD_BUTTON_A) {
        velY = (23.0f / 12.0f) * defaultSpeed * 30.0f;
    }
    player.y += velY * dt;
    player.rotation += (5.0f / 23.0f) * defaultSpeed * 180.0f * dt;

    if (player.y < 15) {
        velY = 0;
        player.y = 15;
        player.rotation = roundf(player.rotation / 90.0f) * 90.0f;
    }

    player.x += velX * dt;

    camX += velX * dt;
    // camX += 1.0f * 30.0f / 60.0f;

    // Calculate view and fade bounds based on camera position
    f32 viewBoundsL = camX - (view_width / 2.0f) / viewScale;
    f32 viewBoundsR = camX + (view_width / 2.0f) / viewScale;

    f32 fadeBoundsL = viewBoundsL + 90.0f;
    f32 fadeBoundsR = viewBoundsR - 90.0f;

    // Build render cache
    RDR_RenderCacheClear(&renderCache);

    for (int i = 0; i < objectsSize; i++) {
        LevelObject* object = &objects[i];
        // first check to see if it's on screen
        if (object->x < viewBoundsL || object->x > viewBoundsR) {
            continue;
        }

        // next check to see if it's a color trigger

        if (
            object->id == 29 ||
            object->id == 30 ||
            object->id == 104 ||
            object->id == 105 ||
            object->id == 744 ||
            object->id == 221 ||
            object->id == 717 ||
            object->id == 718 ||
            object->id == 743 ||
            object->id == 899 ||
            object->is_trigger) {
            if (object->x > lastX && object->x <= camX) {
                u32 color = ((u32)object->red << 24) + ((u32)object->green << 16) + ((u32)object->blue << 8) + ((u32)(object->opacity * 255));
                Color colorObj = { color, object->blending };
                switch (object->id) {
                    case 29:
                        setColorChannel(1000, colorObj);
                        break;
                    case 30:
                        setColorChannel(1001, colorObj);
                        break;
                    case 104:
                        setColorChannel(1002, colorObj);
                        break;
                    case 105:
                        setColorChannel(1004, colorObj);
                        break;
                    case 744:
                        setColorChannel(1003, colorObj);
                        break;
                    case 221:
                        setColorChannel(1, colorObj);
                        break;
                    case 717:
                        setColorChannel(2, colorObj);
                        break;
                    case 718:
                        setColorChannel(3, colorObj);
                        break;
                    case 743:
                        setColorChannel(4, colorObj);
                        break;
                    default:
                        setColorChannel(object->color_channel, colorObj);
                        break;
                }
            }
        }
        // then check to see it it's an animation trigger

        if (object->id == 22 ||
            object->id == 23 ||
            object->id == 24 ||
            object->id == 25 ||
            object->id == 26 ||
            object->id == 27 ||
            object->id == 28 ||
            object->id == 55 ||
            object->id == 56 ||
            object->id == 57 ||
            object->id == 58 ||
            object->id == 59) {
            continue;
        }

        // then do the things that show up in the render cache
        ObjectData* objectData = &objectDefaults[object->id];
        if (!objectData->exists || objectData->texture == NULL) {
            continue;
        }

        texture_info* tex = ht_search(objectData->texture);
        if (tex == NULL) {
            SYS_Report("Can't find texture %d\n", object->id);
            continue;
        }

        RenderObject* renderObject = RDR_RenderCacheAdd(&renderCache);
        if (!renderObject) {
            SYS_Report(__FILE__ ": Cannot create render object!!!\n");
        }
        renderObject->x = object->x;
        renderObject->y = object->y;
        renderObject->rotation = object->rotation;
        renderObject->flipx = object->flipx;
        renderObject->flipy = object->flipy;
        renderObject->z_layer = object->z_layer;
        renderObject->z_order = object->z_order;
        renderObject->z = 0;
        renderObject->tex = tex;

        int baseColorChannel = object->base_color_channel;
        int detailColorChannel = object->detail_color_channel;

        if (objectData->swap_base_detail) {
            int tmp = baseColorChannel;
            baseColorChannel = detailColorChannel;
            detailColorChannel = tmp;
        }

        if (objectData->color_type == COLOR_TYPE_BASE) {
            renderObject->colorChannel = baseColorChannel;
        } else if (objectData->color_type == COLOR_TYPE_DETAIL) {
            renderObject->colorChannel = detailColorChannel;
        } else {
            renderObject->colorChannel = 1010;
        }

        // portals
        char* portalString;
        if ((portalString = isPortalId(object->id))) {
            texture_info* tex = ht_search(portalString);
            if (tex == NULL) {
                SYS_Report("Can't find portal texture %d\n", object->id);
                continue;
            }

            RenderObject* renderObject = RDR_RenderCacheAdd(&renderCache);
            if (!renderObject) {
                SYS_Report(__FILE__ ": Cannot create render object!!!\n");
            }

            renderObject->x = object->x;
            renderObject->y = object->y - 2;
            renderObject->rotation = object->rotation;
            renderObject->flipx = object->flipx;
            renderObject->flipy = object->flipy;
            renderObject->z_layer = 4;
            renderObject->z_order = object->z_order;
            renderObject->z = 0;
            renderObject->tex = tex;

            if (objectData->color_type == COLOR_TYPE_BASE) {
                renderObject->colorChannel = baseColorChannel;
            } else if (objectData->color_type == COLOR_TYPE_DETAIL) {
                renderObject->colorChannel = detailColorChannel;
            } else {
                renderObject->colorChannel = 1010;
            }
        }

        // children part
        ObjectDataChild* children = objectData->children;

        for (int i = 0; i < objectDefaults[object->id].numChildren; i++) {
            ObjectDataChild* child = &children[i];
            texture_info* tex = ht_search(child->texture);
            if (tex == NULL) {
                SYS_Report("Can't find texture %d\n", object->id);
                continue;
            }

            RenderObject* renderObject = RDR_RenderCacheAdd(&renderCache);
            if (!renderObject) {
                SYS_Report(__FILE__ ": Cannot create render object!!!\n");
            }

            renderObject->x = object->x + child->x;
            renderObject->y = object->y + child->y;
            renderObject->rotation = object->rotation + child->rot;
            renderObject->flipx = object->flipx != child->flip_x;
            renderObject->flipy = object->flipy != child->flip_y;
            renderObject->z_layer = object->z_layer;
            renderObject->z_order = object->z_order;
            renderObject->z = child->z;
            renderObject->tex = tex;

            int baseColorChannel = object->base_color_channel;
            int detailColorChannel = object->detail_color_channel;

            if (child->color_type == COLOR_TYPE_BASE) {
                renderObject->colorChannel = baseColorChannel;
            } else if (child->color_type == COLOR_TYPE_DETAIL) {
                renderObject->colorChannel = detailColorChannel;
            } else {
                renderObject->colorChannel = 1010;
            }
        }
    }

    // sort object cache (for layering)
    qsort(renderCache.objects, renderCache.size, sizeof(RenderObject), compare);

    u64 cpuTime = gettime() - allStart;

    // --------------------------------------------------------------------
    // Render graphics
    // --------------------------------------------------------------------

    u64 renderStart = gettime();
    GFX_ResetDrawMode();

    guMtxIdentity(view);

    // Background and ground are special, they don't use the camera view matrix. IT'S ALL AN ILLUSION
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

    // Render all the objects visible on the screen
    int i = 0;
    for (; i < renderCache.size; i++) {
        RenderObject* object = &renderCache.objects[i];

        if (object->z_layer > 4) {
            break;
        }

        if (object->x < fadeBoundsL) {
            f32 dist = fadeBoundsL - viewBoundsL;
            f32 factor = (fadeBoundsL - object->x) / dist;
            object->y -= factor * 90.0f;
        }
        if (object->x > fadeBoundsR) {
            f32 dist = viewBoundsR - fadeBoundsR;
            f32 factor = (object->x - fadeBoundsR) / dist;
            object->y -= factor * 90.0f;
        }

        RDR_drawSpriteFromMap(object->tex, (SpriteInfo){ object->x, object->y, object->rotation, object->flipx, object->flipy }, object->colorChannel, view);
    }

    RDR_drawSpriteFromMap(ht_search("player_01_001.png"), player, 1011, view);
    RDR_drawSpriteFromMap(ht_search("player_01_2_001.png"), player, 1012, view);

    for (; i < renderCache.size; i++) {
        RenderObject* object = &renderCache.objects[i];

        if (object->x < fadeBoundsL) {
            f32 dist = fadeBoundsL - viewBoundsL;
            f32 factor = (fadeBoundsL - object->x) / dist;
            object->y -= factor * 90.0f;
        }
        if (object->x > fadeBoundsR) {
            f32 dist = viewBoundsR - fadeBoundsR;
            f32 factor = (object->x - fadeBoundsR) / dist;
            object->y -= factor * 90.0f;
        }

        RDR_drawSpriteFromMap(object->tex, (SpriteInfo){ object->x, object->y, object->rotation, object->flipx, object->flipy }, object->colorChannel, view);
    }

    // Draw line (VERY IMPORTANT)
    RDR_drawLine(view, camY, viewScale);

    GFX_ResetDrawMode();

    // --------------------------------------------------------------------
    // Debug info
    // --------------------------------------------------------------------
    Mtx model;
    guMtxTrans(model, -(view_width / 2.0f) + 10.0f, view_height / 2.0f - 10.0f, 0.0f);
    char buf[100];
    u64 us = ticks_to_microsecs(lastFrameCPU);
    snprintf(buf, 100, "CPU: %2lld.%03lld ms", us / 1000, us % 1000);
    f32 offset = Font_RenderText(&font, &fontTexture, buf, 16.0f, ALIGN_LEFT, 600.0f, model);

    us = ticks_to_microsecs(lastFrameRender);
    snprintf(buf, 100, "RDR: %2lld.%03lld ms", us / 1000, us % 1000);
    guMtxTransApply(model, model, 0.0f, offset, 0.0f);
    offset = Font_RenderText(&font, &fontTexture, buf, 16.0f, ALIGN_LEFT, 600.0f, model);

    us = ticks_to_microsecs(lastFrameTotal);
    snprintf(buf, 100, "ALL: %2lld.%03lld ms", us / 1000, us % 1000);
    guMtxTransApply(model, model, 0.0f, offset, 0.0f);
    offset = Font_RenderText(&font, &fontTexture, buf, 16.0f, ALIGN_LEFT, 600.0f, model);

    snprintf(buf, 100, "Objects: %d/%d (%d/%d bytes)", renderCache.size, objectsSize, renderCache.capacity * sizeof(LevelObject), objectsSize * sizeof(LevelObject));
    guMtxTransApply(model, model, 0.0f, offset, 0.0f);
    offset = Font_RenderText(&font, &fontTexture, buf, 16.0f, ALIGN_LEFT, 600.0f, model);

    snprintf(buf, 100, "Position: (%.3f, %.3f), velocity: (%.3f, %.3f)", player.x, player.y, velX, velY);
    guMtxTransApply(model, model, 0.0f, offset, 0.0f);
    offset = Font_RenderText(&font, &fontTexture, buf, 16.0f, ALIGN_LEFT, 600.0f, model);

    snprintf(buf, 100, "Current level: %s (%d)", levelNames[currentLevelId], currentLevelId + 1);
    guMtxTransApply(model, model, 0.0f, offset, 0.0f);
    offset = Font_RenderText(&font, &fontTexture, buf, 20.0f, ALIGN_LEFT, 600.0f, model);

    u64 renderTime = gettime() - renderStart;
    u64 totalTime = gettime() - allStart;

    lastFrameCPU = cpuTime;
    lastFrameRender = renderTime;
    lastFrameTotal = totalTime;

    // if (HWButton != -1) {
    //     free(objects);
    //     SYS_ResetSystem(SYS_POWEROFF, 0, 0);
    //     // SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
    // }
}