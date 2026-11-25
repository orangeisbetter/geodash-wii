#pragma once

#include <gccore.h>

typedef struct {
    u32 color;
    bool blending;
} Color;

typedef struct {
    u16 colorId;
    Color color;
} ColorData;

typedef struct {
    int length;
    ColorData colors[16];
} ColorConfig;

void colorInit(ColorConfig colorConfig);

Color getColorChannel(int channel);

void setColorChannel(int channel, Color newColor, f32 duration);

void updateColorChannels(f32 dt);

void colorConfigPush(ColorConfig* config, int channel, Color color);
