#pragma once

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <gccore.h>
#include <malloc.h>
#include <math.h>

#include "gfx.h"

typedef struct {
    u32 size;
} BM_Info;

typedef struct {
    u32 lineHeight;
    u32 base;
    u32 scaleW;
    u32 scaleH;
    u8 pages;
} BM_Common;

typedef struct {
    bool valid;
    u16 x;
    u16 y;
    u8 width;
    u8 height;
    s8 xoffset;
    s8 yoffset;
    s8 xadvance;
} BM_Char;

typedef struct {
    u8 first;
    u8 second;
    s8 amount;
} BM_Kerning;

typedef struct {
    u8 id;
    char* file;
    BM_Char chars[128];
    BM_Kerning* kernings;
    int numKernings;
} BM_Page;

typedef struct {
    BM_Info info;
    BM_Common common;
    BM_Page page;
} BMFont;

typedef enum {
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT,
} Font_Alignment;

typedef struct {
    char* ptr;
    int length;
    int width;
} Font_Word;

int parseBMFontFile(const char* filepath, BMFont* font);
void Font_RenderGlyph(BMFont* font, f32* posx, f32* posy, char character);
void Font_RenderLine(BMFont* font, f32 fontSize, Font_Alignment align, Mtx modelView, f32* posx, f32* posy, char* line, int length, int width);
Font_Word trimWord(BMFont* font, Font_Word word);
f32 Font_RenderText(BMFont* font, GXTexObj* fontTexture, char* text, f32 fontSize, Font_Alignment align, f32 maxWidth, Mtx modelView);