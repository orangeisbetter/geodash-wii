#include "pointer.h"

#include <wiiuse/wpad.h>

#include "gfx.h"
#include "util/hash_table.h"

#define POINTER_TEXTURE "shard0201ShardBig_001.png"

Pointer pointer = { 0 };

void PTR_Init(){
    WPAD_SetDataFormat(WPAD_CHAN_0, WPAD_FMT_BTNS_ACC_IR);
    WPAD_SetVRes(WPAD_CHAN_0, view_width, view_height);
}

void PTR_Update(){
    ir_t ir;
    WPAD_IR(WPAD_CHAN_0, &ir);

    pointer.valid = ir.valid;
    if(!ir.valid){
        return;
    }

    pointer.x = ir.x - view_width / 2.0f;
    pointer.y = view_height / 2.0f - ir.y;
    pointer.angle = ir.angle;
}

void PTR_Draw(f32 viewScale){
    texture_info* tex = ht_search(POINTER_TEXTURE);

    Mtx view;
    guMtxScale(view, viewScale, viewScale, 1.0f);

    SpriteInfo sprite = {
        pointer.x / viewScale,
        pointer.y / viewScale,
        pointer.angle,
        1.0f,
        false,
        false,
    };

    RDR_drawSpriteFromMap2(tex, &sprite, 0xffffffff, false, view);
}

bool PTR_IsOverSprite(const texture_info* tex, const SpriteInfo* sprite, f32 viewScale) {
    if (!tex || !pointer.valid) {
        return false;
    }

    f32 x = pointer.x / viewScale;
    f32 y = pointer.y / viewScale;

    f32 centerX = sprite->x + tex->spriteOffset.x;
    f32 centerY = sprite->y + tex->spriteOffset.y;
    f32 halfW = tex->spriteSize.x / 2.0f;
    f32 halfH = tex->spriteSize.y / 2.0f;

    return x >= centerX - halfW && x <= centerX + halfW &&
           y >= centerY - halfH && y <= centerY + halfH;
}