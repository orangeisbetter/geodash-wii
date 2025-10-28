#pragma once

#include <gccore.h>
#include "util/hash_table.h"
#include "BMFont.h"

typedef struct {
    int width;
    int height;
} TexSize;

typedef struct {
    texture_info* tex;
    f32 x;
    f32 y;
    bool flipx;
    bool flipy;
    f32 rotation;
    u16 colorChannel;
    u8 z_layer;
    s32 z_order;
    s32 z;
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

void RDR_drawSpriteFromMap(texture_info* tex, SpriteInfo sprite, int colorChannel, Mtx view);

int RDR_RenderCacheInit(RenderCache* renderCache, int initialCapacity);
RenderObject* RDR_RenderCacheAdd(RenderCache* renderCache);
void RDR_RenderCacheClear(RenderCache* renderCache);
void RDR_RenderCacheFree(RenderCache* renderCache);