#include "render.h"

#include <malloc.h>
#include <ogc/lwp_watchdog.h>

#include "constants.h"
#include "gfx.h"
#include "color.h"
#include "util/hash_table.h"

GXTexObj backgroundTexture;
GXTexObj groundTexture;
GXTexObj spriteSheet;
GXTexObj gameSheet02;
GXTexObj gameSheet03;
GXTexObj gameSheet04;
GXTexObj gameSheetIcons;
GXTexObj gameSheetGlow;
BMFont font;
GXTexObj fontTexture;

static TPLFile spriteTPL;
static TexSize texSizes[6];

int RDR_loadSpriteSheets() {
    if (TPL_OpenTPLFromFile(&spriteTPL, ASSETS_PATH "GJ_GameSheet.tpl") != 1) {
        return 0;
    }

    TPL_GetTexture(&spriteTPL, 0, &spriteSheet);
    texSizes[0] = (TexSize){
        GX_GetTexObjWidth(&spriteSheet),
        GX_GetTexObjHeight(&spriteSheet)
    };

    TPL_GetTexture(&spriteTPL, 1, &gameSheet02);
    texSizes[1] = (TexSize){
        GX_GetTexObjWidth(&gameSheet02),
        GX_GetTexObjHeight(&gameSheet02)
    };

    TPL_GetTexture(&spriteTPL, 2, &gameSheet03);
    texSizes[2] = (TexSize){
        GX_GetTexObjWidth(&gameSheet03),
        GX_GetTexObjHeight(&gameSheet03)
    };

    TPL_GetTexture(&spriteTPL, 3, &gameSheet04);
    texSizes[3] = (TexSize){
        GX_GetTexObjWidth(&gameSheet04),
        GX_GetTexObjHeight(&gameSheet04)
    };

    TPL_GetTexture(&spriteTPL, 4, &gameSheetIcons);
    texSizes[4] = (TexSize){
        GX_GetTexObjWidth(&gameSheetIcons),
        GX_GetTexObjHeight(&gameSheetIcons)
    };

    TPL_GetTexture(&spriteTPL, 5, &gameSheetGlow);
    texSizes[5] = (TexSize){
        GX_GetTexObjWidth(&gameSheetGlow),
        GX_GetTexObjHeight(&gameSheetGlow)
    };

    return 1;
}

TexSize RDR_getTexSize(int sheetId) {
    return texSizes[sheetId];
}

void RDR_drawBackground(GXTexObj* background, f32 x, f32 y, f32 scale, u32 color) {
    x /= 10000.0f;
    y /= 10000.0f;

    f32 top = 1.0f - y - scale;
    f32 bottom = 1.0f - y;
    f32 left = x;
    f32 right = x + scale * aspect_ratio;

    f32 w = view_width / 2.0f;
    f32 h = view_height / 2.0f;

    Mtx model;
    guMtxIdentity(model);
    guMtxTransApply(model, model, 0.0f, 0.0f, -100.0f);
    GX_LoadPosMtxImm(model, GX_PNMTX0);

    GFX_LoadTexture(background, GX_TEXMAP0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);

    GFX_SetBlendMode(BLEND_NONE);

    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);

    GFX_Plot(-w, h, 0.0f, left, top, color);
    GFX_Plot(w, h, 0.0f, right, top, color);
    GFX_Plot(w, -h, 0.0f, right, bottom, color);
    GFX_Plot(-w, -h, 0.0f, left, bottom, color);

    GX_End();
}

void RDR_drawGround(GXTexObj* ground, f32 x, f32 y, f32 viewScale, u32 color) {
    x = -x;
    // x *= viewScale;
    y = -y;
    y *= viewScale;

    f32 w2 = view_width / 2.0f;

    f32 texw = (view_width / 128.0f) / viewScale;

    f32 texXOffset = x / 128.0f;

    Mtx model;
    guMtxIdentity(model);
    guMtxTransApply(model, model, 0.0f, 0.0f, -5.0f);
    GX_LoadPosMtxImm(model, GX_PNMTX0);

    GFX_LoadTexture(ground, GX_TEXMAP0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);

    GFX_SetBlendMode(BLEND_NONE);

    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);

    GFX_Plot(-w2, y, 0.0f, -texXOffset, 0.0f, color);
    GFX_Plot(w2, y, 0.0f, -texXOffset + texw, 0.0f, color);
    GFX_Plot(w2, y - 128.0f * viewScale, 0.0f, -texXOffset + texw, 1.0f, color);
    GFX_Plot(-w2, y - 128.0f * viewScale, 0.0f, -texXOffset, 1.0f, color);

    GX_End();
}

void RDR_drawMenuBackground(f32 x, f32 y, f32 scale, u32 topColor, u32 bottomColor) {
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

    GFX_SetBlendMode(BLEND_NONE);

    Mtx model;
    guMtxIdentity(model);
    guMtxTransApply(model, model, 0.0f, 0.0f, -100.0f);
    GX_LoadPosMtxImm(model, GX_PNMTX0);

    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);

    GX_Position3f32(-w, h, 0.0f);
    GX_Color1u32(topColor);

    GX_Position3f32(w, h, 0.0f);
    GX_Color1u32(topColor);

    GX_Position3f32(w, -h, 0.0f);
    GX_Color1u32(bottomColor);

    GX_Position3f32(-w, -h, 0.0f);
    GX_Color1u32(bottomColor);

    GX_End();

    GFX_ResetDrawMode();
}

void RDR_drawLine(Mtx view, f32 camY, f32 viewScale) {
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);

    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

    guMtxTrans(view, 0.0f, -camY, 0.0f);
    guMtxScaleApply(view, view, 1.0f, viewScale, 1.0f);
    GX_LoadPosMtxImm(view, GX_PNMTX0);

    GX_SetNumChans(1);
    GX_SetNumTexGens(0);

    GFX_SetBlendMode(BLEND_ALPHA);

    GX_Begin(GX_QUADS, GX_VTXFMT0, 12);

    f32 halfw = view_width / 2.8f;
    f32 innerw = view_width / 8.0f / 2.0f;

    f32 thickness = 1.2f / 2.0f;

    // left part
    GX_Position3f32(-halfw, thickness, 0.0f);
    GX_Color1u32(0xffffff00);

    GX_Position3f32(-innerw, thickness, 0.0f);
    GX_Color1u32(0xffffffff);

    GX_Position3f32(-innerw, -thickness, 0.0f);
    GX_Color1u32(0xffffffff);

    GX_Position3f32(-halfw, -thickness, 0.0f);
    GX_Color1u32(0xffffff00);

    // center part
    GX_Position3f32(-innerw, thickness, 0.0f);
    GX_Color1u32(0xffffffff);

    GX_Position3f32(+innerw, thickness, 0.0f);
    GX_Color1u32(0xffffffff);

    GX_Position3f32(+innerw, -thickness, 0.0f);
    GX_Color1u32(0xffffffff);

    GX_Position3f32(-innerw, -thickness, 0.0f);
    GX_Color1u32(0xffffffff);

    // right part
    GX_Position3f32(+innerw, thickness, 0.0f);
    GX_Color1u32(0xffffffff);

    GX_Position3f32(+halfw, thickness, 0.0f);
    GX_Color1u32(0xffffff00);

    GX_Position3f32(+halfw, -thickness, 0.0f);
    GX_Color1u32(0xffffff00);

    GX_Position3f32(+innerw, -thickness, 0.0f);
    GX_Color1u32(0xffffffff);

    GX_End();
}

void RDR_drawSpriteFromMap(texture_info* tex, SpriteInfo sprite, int colorChannel, Mtx view) {
    Color color = getColorChannel(colorChannel);
    RDR_drawSpriteFromMap2(tex, sprite, color.color, color.blending, view);
}

void RDR_drawSpriteFromMap2(texture_info* tex, SpriteInfo sprite, u32 color, bool blending, Mtx view) {
    if (blending) {
        GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_NOOP);
    } else {
        GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    }

    f32 dx = sprite.x;
    f32 dy = sprite.y;
    f32 dw = tex->spriteSize.x;
    f32 dh = tex->spriteSize.y;

    f32 offsetx = tex->spriteOffset.x;
    f32 offsety = tex->spriteOffset.y;

    f32 sizex = tex->textureRotated ? tex->textureRect.size.y : tex->textureRect.size.x;
    f32 sizey = tex->textureRotated ? tex->textureRect.size.x : tex->textureRect.size.y;

    f32 u0 = tex->textureRect.pos.x / (f32)RDR_getTexSize(tex->sheetIdx).width;
    f32 u1 = u0 + sizex / (f32)RDR_getTexSize(tex->sheetIdx).width;
    f32 v0 = tex->textureRect.pos.y / (f32)RDR_getTexSize(tex->sheetIdx).height;
    f32 v1 = v0 + sizey / (f32)RDR_getTexSize(tex->sheetIdx).height;

    f32 halfW = dw / 2;
    f32 halfH = dh / 2;

    if (sprite.flipx) {
        halfW *= -1;
        offsetx *= -1;
    }
    if (sprite.flipy) {
        halfH *= -1;
        offsety *= -1;
    }

    static guVector forward = { 0.0f, 0.0f, -1.0f };

    Mtx model, modelView;
    guMtxRotAxisDeg(model, &forward, sprite.rotation);
    guMtxApplyTrans(model, model, tex->spriteOffset.x, tex->spriteOffset.y, 0.0f);
    guMtxTransApply(model, model, dx, dy, 0.0f);
    guMtxConcat(view, model, modelView);
    GX_LoadPosMtxImm(modelView, GX_PNMTX0);

    GFX_BindTexture(tex->sheetIdx);

    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);

    if (tex->textureRotated) {
        GFX_Plot(-halfW, +halfH, 0.0f, u1, v0, color);
        GFX_Plot(+halfW, +halfH, 0.0f, u1, v1, color);
        GFX_Plot(+halfW, -halfH, 0.0f, u0, v1, color);
        GFX_Plot(-halfW, -halfH, 0.0f, u0, v0, color);
    } else {
        GFX_Plot(-halfW, +halfH, 0.0f, u0, v0, color);
        GFX_Plot(+halfW, +halfH, 0.0f, u1, v0, color);
        GFX_Plot(+halfW, -halfH, 0.0f, u1, v1, color);
        GFX_Plot(-halfW, -halfH, 0.0f, u0, v1, color);
    }

    GX_End();
}

void RDR_drawRect(GXTexObj* tex, f32 x, f32 y, f32 w, f32 h, u32 color) {
    f32 width = GX_GetTexObjWidth(tex);
    f32 height = GX_GetTexObjHeight(tex);

    f32 texWidthFactor = 0.5f;
    f32 texHeightFactor = 0.5f;

    if (h < height) {
        texHeightFactor = h / height / 2.0;
        height = h;
    }
    if (w < width) {
        texWidthFactor = w / width / 2.0;
        width = w;
    }

    // SYS_Report("width factor: %f, height factor: %f\n", texWidthFactor, texHeightFactor);

    Mtx model;
    guMtxIdentity(model);
    guMtxTransApply(model, model, 0.0f, 0.0f, -1.0f);
    GX_LoadPosMtxImm(model, GX_PNMTX0);

    GFX_LoadTexture(tex, GX_TEXMAP0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);

    GFX_SetBlendMode(BLEND_ALPHA);

    f32 w2 = w / 2.0f;
    f32 h2 = h / 2.0f;

    // topleft
    GX_Begin(GX_QUADS, GX_VTXFMT0, 16);

    GFX_Plot(-w2 + x, h2 + y, 0.0f, 0.0f, 0.0f, color);
    GFX_Plot(-w2 + width / 2 + x, h2 + y, 0.0f, texWidthFactor, 0.0f, color);
    GFX_Plot(-w2 + width / 2 + x, h2 - height / 2 + y, 0.0f, texWidthFactor, texHeightFactor, color);
    GFX_Plot(-w2 + x, h2 - height / 2 + y, 0.0f, 0.0f, texHeightFactor, color);

    GFX_Plot(w2 - width / 2 + x, h2 + y, 0.0f, 1 - texWidthFactor, 0.0f, color);
    GFX_Plot(w2 + x, h2 + y, 0.0f, 1.0f, 0.0f, color);
    GFX_Plot(w2 + x, h2 - height / 2 + y, 0.0f, 1.0f, texHeightFactor, color);
    GFX_Plot(w2 - width / 2 + x, h2 - height / 2 + y, 0.0f, 1 - texWidthFactor, texHeightFactor, color);

    GFX_Plot(w2 - width / 2 + x, -h2 + height / 2 + y, 0.0f, 1 - texWidthFactor, 1 - texHeightFactor, color);
    GFX_Plot(w2 + x, -h2 + height / 2 + y, 0.0f, 1.0f, 1 - texHeightFactor, color);
    GFX_Plot(w2 + x, -h2 + y, 0.0f, 1.0f, 1.0f, color);
    GFX_Plot(w2 - width / 2 + x, -h2 + y, 0.0f, 1 - texWidthFactor, 1.0f, color);

    GFX_Plot(-w2 + x, -h2 + height / 2 + y, 0.0f, 0.0f, 1 - texHeightFactor, color);
    GFX_Plot(-w2 + width / 2 + x, -h2 + height / 2 + y, 0.0f, texWidthFactor, 1 - texHeightFactor, color);
    GFX_Plot(-w2 + width / 2 + x, -h2 + y, 0.0f, texWidthFactor, 1.0f, color);
    GFX_Plot(-w2 + x, -h2 + y, 0.0f, 0.0f, 1.0f, color);

    GX_End();

    GX_Begin(GX_QUADS, GX_VTXFMT0, 20);
    // left
    GFX_Plot(-w2 + x, h2 - height / 2 + y, 0.0f, 0.0f, 0.25f, color);
    GFX_Plot(-w2 + width / 2 + x, h2 - height / 2 + y, 0.0f, texWidthFactor, 0.25f, color);
    GFX_Plot(-w2 + width / 2 + x, -h2 + height / 2 + y, 0.0f, texWidthFactor, 0.75f, color);
    GFX_Plot(-w2 + x, -h2 + height / 2 + y, 0.0f, 0.0f, 0.75f, color);
    // top
    GFX_Plot(-w2 + width / 2 + x, h2 + y, 0.0f, 0.25f, 0.0f, color);
    GFX_Plot(w2 - width / 2 + x, h2 + y, 0.0f, 0.75f, 0.0f, color);
    GFX_Plot(w2 - width / 2 + x, h2 - height / 2 + y, 0.0f, 0.75f, texHeightFactor, color);
    GFX_Plot(-w2 + width / 2 + x, h2 - height / 2 + y, 0.0f, 0.25f, texHeightFactor, color);
    // right
    GFX_Plot(w2 - width / 2 + x, h2 - height / 2 + y, 0.0f, 1 - texWidthFactor, 0.25f, color);
    GFX_Plot(w2 + x, h2 - height / 2 + y, 0.0f, 1.0f, 0.25f, color);
    GFX_Plot(w2 + x, -h2 + height / 2 + y, 0.0f, 1.0f, 0.75f, color);
    GFX_Plot(w2 - width / 2 + x, -h2 + height / 2 + y, 0.0f, 1 - texWidthFactor, 0.75f, color);
    // bottom
    GFX_Plot(-w2 + width / 2 + x, -h2 + height / 2 + y, 0.0f, 0.25f, 1 - texHeightFactor, color);
    GFX_Plot(w2 - width / 2 + x, -h2 + height / 2 + y, 0.0f, 0.75f, 1 - texHeightFactor, color);
    GFX_Plot(w2 - width / 2 + x, -h2 + y, 0.0f, 0.75f, 1.0f, color);
    GFX_Plot(-w2 + width / 2 + x, -h2 + y, 0.0f, 0.25f, 1.0f, color);
    // center
    GFX_Plot(-w2 + width / 2 + x, h2 - height / 2 + y, 0.0f, 0.25f, 0.25f, color);
    GFX_Plot(w2 - width / 2 + x, h2 - height / 2 + y, 0.0f, 0.75f, 0.25f, color);
    GFX_Plot(w2 - width / 2 + x, -h2 + height / 2 + y, 0.0f, 0.75f, 0.75f, color);
    GFX_Plot(-w2 + width / 2 + x, -h2 + height / 2 + y, 0.0f, 0.25f, 0.75f, color);

    GX_End();
}

int RDR_RenderCacheInit(RenderCache* renderCache, int initialCapacity) {
    renderCache->size = 0;
    renderCache->objects = malloc(sizeof(RenderObject) * initialCapacity);
    if (renderCache->objects == NULL) {
        return 0;
    }
    renderCache->capacity = initialCapacity;
    return 1;
}

RenderObject* RDR_RenderCacheAdd(RenderCache* renderCache) {
    if (renderCache->size == renderCache->capacity) {
        renderCache->capacity *= 2;
        SYS_Report("Resizing object cache size to %d objects\n", renderCache->capacity);
        RenderObject* newObjects = realloc(renderCache->objects, sizeof(RenderObject) * renderCache->capacity);
        if (newObjects == NULL) {
            SYS_Report("Out of memory, render cache cannot be resized\n");
            return NULL;
        }
        renderCache->objects = newObjects;
    }
    return renderCache->objects + (renderCache->size++);
}

void RDR_RenderCacheClear(RenderCache* renderCache) {
    renderCache->size = 0;
}

void RDR_RenderCacheFree(RenderCache* renderCache) {
    renderCache->size = 0;
    renderCache->capacity = 0;
    free(renderCache->objects);
    renderCache->objects = NULL;
}
