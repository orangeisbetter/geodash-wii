#pragma once

#include <gccore.h>

typedef enum {
    BLEND_NONE,
    BLEND_ALPHA,
    BLEND_STENCIL,
} GFX_BlendMode;

extern GXRModeObj* rmode;

extern u16 view_width;
extern u16 view_height;
extern f32 aspect_ratio;
extern bool widescreen;
extern u8 refresh_rate;

void GFX_InitVideo();
void GFX_Init();
void GFX_SwapBuffers();

static inline void GFX_ResetDrawMode() {
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

    GX_SetColorUpdate(GX_TRUE);
    GX_SetAlphaUpdate(GX_FALSE);
    GX_SetZMode(GX_ENABLE, GX_LEQUAL, GX_ENABLE);

    // Basic TEV setup - outputcolor = inputcolor * texcolor
    GX_SetNumChans(1);
    GX_SetNumTexGens(6);

    GX_SetNumTevStages(1);
    GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
}

static inline void GFX_SetBlendMode(GFX_BlendMode mode) {
    switch (mode) {
        case BLEND_NONE:
            GX_SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
            GX_SetZCompLoc(GX_TRUE);
            GX_SetBlendMode(GX_BM_NONE, GX_BL_ZERO, GX_BL_ZERO, GX_LO_NOOP);
            break;
        case BLEND_ALPHA:
            GX_SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
            GX_SetZCompLoc(GX_TRUE);
            GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
            break;
        case BLEND_STENCIL:
            GX_SetAlphaCompare(GX_GREATER, 128, GX_AOP_AND, GX_ALWAYS, 0);
            GX_SetZCompLoc(GX_FALSE);
            GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
            break;
    }
}

static inline void GFX_LoadTexture(GXTexObj* tex, u8 texmap) {
    GX_LoadTexObj(tex, texmap);
    GX_SetTexCoordGen(GX_TEXCOORD0 + texmap, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0 + texmap, GX_TEXMAP0 + texmap, GX_COLOR0A0);
}

static inline void GFX_BindTexture(u8 texmap) {
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0 + texmap, GX_TEXMAP0 + texmap, GX_COLOR0A0);
}

static inline void GFX_Plot(f32 x, f32 y, f32 z, f32 s, f32 t, u32 c) {
    GX_Position3f32(x, y, z);
    GX_Color1u32(c);
    GX_TexCoord2f32(s, t);
}