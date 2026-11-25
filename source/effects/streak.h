#pragma once

#include <gccore.h>

void STK_Reset();

void STK_AddPoint(f32 x, f32 y);

void STK_Update(f32 dt);
void STK_Draw(const Mtx view, f32 mirrorCenter, f32 mirrorScale);