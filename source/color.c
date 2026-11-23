#include "color.h"

typedef struct{
    Color oldColor;
    Color newColor;
    f32 duration;
    f32 timeElapsed;
} ColorTransitionData;

static Color colors[1024];
static ColorTransitionData colorTransitionData[1024];

void colorInit(ColorConfig colorConfig) {
    for (int i = 0; i < 1000; i++) {
        colors[i].color = 0xffffffff;
        colors[i].blending = false;
    }
    
    colors[1000].color = 0x287dffff;
    colors[1000].blending = false;

    colors[1001].color = 0x0066ffff;
    colors[1001].blending = false;

    colors[1002].color = 0xffffffff;
    colors[1002].blending = true;

    colors[1003].color = 0xffffffff;
    colors[1003].blending = false;

    colors[1004].color = 0xffffffff;
    colors[1004].blending = false;

    // colors[1005].color = 0xfbceffff;
    colors[1005].color = 0x00ff00ff;
    colors[1005].blending = true;

    // colors[1006].color = 0x7630fdff;
    colors[1006].color = 0x00ffffff;
    colors[1006].blending = true;

    colors[1007].color = 0xffff00ff;
    colors[1007].blending = false;

    colors[1009].color = 0x0066ffff;
    colors[1009].blending = false;

    colors[1010].color = 0x000000ff;
    colors[1010].blending = false;

    colors[1011].color = 0x00ff00ff;
    colors[1011].blending = false;

    colors[1012].color = 0x00ffffff;
    colors[1012].blending = false;

    colors[1013].color = 0x287dffff;
    colors[1013].blending = false;

    colors[1014].color = 0x287dffff;
    colors[1014].blending = false;

    for(int i=0; i<colorConfig.length; i++){
        colors[colorConfig.colors[i].colorId] = colorConfig.colors[i].color;
    }
    for(int i=0; i<1024; i++){
        colorTransitionData[i] = (ColorTransitionData){colors[i], colors[i], 0.5f, 0.5f};
    }
}

Color getColorChannel(int channel) {
    return colors[channel];
}

static u32 lerpColor(u32 from, u32 to, f32 t) {
    u32 result = 0;
    for (int shift = 0; shift < 32; shift += 8) {
        s32 a = (from >> shift) & 0xff;
        s32 b = (to >> shift) & 0xff;
        result |= (u32)(a + (b - a) * t) << shift;
    }
    return result;
}

void setColorChannel(int channel, Color newColor, f32 duration) {
    ColorTransitionData* transition = &colorTransitionData[channel];

    if (duration <= 0.0f) {
        // Instant change
        colors[channel] = newColor;
        transition->duration = 0.0f;
        transition->timeElapsed = 0.0f;
        return;
    }

    transition->oldColor = colors[channel];
    transition->newColor = newColor;
    transition->duration = duration;
    transition->timeElapsed = 0.0f;

    colors[channel].blending = newColor.blending;
}

void updateColorChannels(f32 dt){
    for(int i=0; i<1024; i++){
        ColorTransitionData* transition = &colorTransitionData[i];
        if (transition->timeElapsed >= transition->duration) {
            continue;
        }
        
        transition->timeElapsed += dt;

        f32 t = transition->timeElapsed / transition->duration;
        if (t > 1.0f) {
            t = 1.0f;
        }

        colors[i].color = lerpColor(transition->oldColor.color, transition->newColor.color, t);
    }
}