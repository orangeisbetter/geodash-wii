#include "color.h"

#include "util/pool.h"

#define MAX_NUM_TRANSITIONS 8

typedef struct {
    int colorChannel;
    u32 oldColor;
    u32 newColor;
    f32 duration;
    f32 timeElapsed;
} ColorTransition;

static struct {
    ColorTransition transitions[MAX_NUM_TRANSITIONS];
    PoolSlot poolSlots[MAX_NUM_TRANSITIONS];
    PoolSlot* activeList;
    Pool pool;
} transitionPool;

static Color colors[1024];

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

    // Initialize & reset color transitions
    Pool_Init(&transitionPool.pool, transitionPool.poolSlots, MAX_NUM_TRANSITIONS);
    transitionPool.activeList = NULL;
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

// return -1 if not, otherwise pool slot index
static int inTransition(int channel) {
    for (PoolSlot* current = transitionPool.activeList; current; current = current->next) {
        unsigned int index = current - transitionPool.poolSlots;
        ColorTransition* transition = transitionPool.transitions + index;
        if (channel == transition->colorChannel) {
            return index;
        }
    }
    return -1;
}

void setColorChannel(int channel, Color newColor, f32 duration) {
    // If this channel is already in transition, cancel that transition
    unsigned int index = inTransition(channel);
    if (index != -1) {
        Pool_Release(&transitionPool.pool, index, &transitionPool.activeList);
    }

    // Blending always applies instantly; no way to interpolate that
    colors[channel].blending = newColor.blending;

    // Transition of time zero is instant; just do it now
    if (duration == 0.0f) {
        colors[channel].color = newColor.color;
        return;
    }

    int slotIndex = Pool_Acquire(&transitionPool.pool, &transitionPool.activeList);

    // If we didn't get a slot, just fall back to an instant transition. If this becomes a problem, just increase
    // the number of slots
    if (slotIndex == -1) {
        colors[channel].color = newColor.color;
    }

    // Initialize transition info
    ColorTransition* transition = transitionPool.transitions + slotIndex;
    transition->colorChannel = channel;
    transition->oldColor = colors[channel].color;
    transition->newColor = newColor.color;
    transition->duration = duration;
    transition->timeElapsed = 0.0f;
}

void updateColorChannels(f32 dt){
    PoolSlot* current = transitionPool.activeList;
    while (current) {
        unsigned int index = current - transitionPool.poolSlots;
        ColorTransition* transition = transitionPool.transitions + index;

        transition->timeElapsed += dt;

        f32 t = transition->timeElapsed / transition->duration;
        if (t > 1.0f) {
            t = 1.0f;
        }

        colors[transition->colorChannel].color = lerpColor(transition->oldColor, transition->newColor, t);

        if (t == 1.0f) {
            // Transition finished; release it
            PoolSlot* next = current->next;
            Pool_Release(&transitionPool.pool, index, &transitionPool.activeList);
            current = next;
        } else {
            current = current->next;
        }
    }
}
