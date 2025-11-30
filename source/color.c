#include "color.h"

static Color colors[1024];

void colorInit() {
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
}

Color getColorChannel(int channel) {
    return colors[channel];
}

void setColorChannel(int channel, Color newColor) {
    colors[channel] = newColor;
}