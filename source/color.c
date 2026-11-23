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

void setColorChannel(int channel, Color newColor, f32 duration) {
    colorTransitionData[channel].oldColor = colors[channel];
    colorTransitionData[channel].newColor = newColor;
    colorTransitionData[channel].duration = duration;
    // colors[channel] = newColor;
}

void updateColorChannels(f32 dt){
    for(int i=0; i<1024; i++){
        if(colorTransitionData[i].timeElapsed < colorTransitionData[i].duration){
            SYS_Report("simply putting code inside this if statement causes stuff not to be rendered regardless of if it's even getting called!!!\n");
            // colors[i].blending = colorTransitionData[i].newColor.blending;
            // colorTransitionData[i].timeElapsed += dt;
            // colors[i].color = (colorTransitionData[i].newColor.color - colorTransitionData[i].oldColor.color) * colorTransitionData[i].timeElapsed / colorTransitionData[i].duration + colorTransitionData[i].newColor.color;
        }
    }
}