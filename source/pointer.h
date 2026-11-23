#pragma once

#include <gccore.h>

#include "render.h"

typedef struct {
    f32 x;      // Screen space, origin at center, y up
    f32 y;
    f32 angle;  // Wiimote roll in degrees
    bool valid; // False when the sensor bar is not visible
} Pointer;

extern Pointer pointer;

void PTR_Init();
void PTR_Update();
void PTR_Draw(f32 viewScale);
bool PTR_IsOverSprite(const texture_info* tex, const SpriteInfo* sprite, f32 viewScale);