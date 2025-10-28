#pragma once

#include <gccore.h>

typedef struct {
    u32 color;
    bool blending;
} Color;

void colorInit();

Color getColorChannel(int channel);

void setColorChannel(int channel, Color newColor);