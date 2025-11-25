#pragma once

#include <gccore.h>
#include "util/hash_table.h"
#include "parse/level.h"
#include "BMFont.h"
#include "color.h"

typedef struct {
    int width;
    int height;
} TexSize;

typedef struct {
    texture_info* tex;
    f32 x;
    f32 y;
    f32 rotation;
    f32 transform[2][3];
    // f32 scaleX;
    // f32 scaleY;
    s32 z_order;
    u8 z_layer;
    s32 z;
    Color color;
    bool flipx;
    bool flipy;
} RenderObject;

typedef struct {
    f32 x;
    f32 y;
    f32 rotation;
    bool flipx;
    bool flipy;
} SpriteInfo;

typedef struct {
    RenderObject* objects;
    int size;
    int capacity;
} RenderCache;

extern GXTexObj spriteSheet;
extern GXTexObj gameSheet02;
extern GXTexObj gameSheet03;
extern GXTexObj gameSheet04;
extern GXTexObj gameSheetIcons;
extern GXTexObj gameSheetGlow;

extern GXTexObj backgroundTexture;
extern GXTexObj groundTexture;

extern BMFont font;
extern GXTexObj fontTexture;

int RDR_loadSpriteSheets();

TexSize RDR_getTexSize(int sheetId);

void RDR_drawBackground(GXTexObj* background, f32 x, f32 y, f32 scale, u32 color);
void RDR_drawGround(GXTexObj* ground, f32 x, f32 y, f32 viewScale, u32 color);
void RDR_drawLine(Mtx view, f32 camY, f32 viewScale);
void RDR_drawMenuBackground(f32 x, f32 y, f32 scale, u32 topColor, u32 bottomColor);

void RDR_drawSpriteFromMap(texture_info* tex, SpriteInfo* sprite, int colorChannel, Mtx view);
void RDR_drawSpriteFromMap2(texture_info* tex, SpriteInfo* sprite, u32 color, bool blending, Mtx view);
void RDR_drawRenderObject(RenderObject* object, bool flipped, Mtx view);
// void RDR_drawLevelObject(LevelObject* object, Mtx view);
void RDR_drawRect(GXTexObj* tex, f32 x, f32 y, f32 w, f32 h, u32 color);

int RDR_RenderCacheInit(RenderCache* renderCache, int initialCapacity);
RenderObject* RDR_RenderCacheAdd(RenderCache* renderCache);
void RDR_RenderCacheClear(RenderCache* renderCache);
void RDR_RenderCacheFree(RenderCache* renderCache);