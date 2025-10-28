#pragma once
#include <stdint.h>
#include <gccore.h>

typedef enum {
    COLOR_TYPE_BASE,
    COLOR_TYPE_DETAIL,
    COLOR_TYPE_BLACK
} ColorType;

typedef struct {
    char* texture;
    float anchor_x;
    float anchor_y;
    float scale_x;
    float scale_y;
    bool flip_x;
    bool flip_y;
    float x;
    float y;
    int z;
    float rot;
    ColorType color_type;
} ObjectDataChild;

typedef struct {
    bool exists;
    char* texture;
    uint8_t default_z_layer;
    int default_z_order;
    int default_base_color_channel;
    int default_detail_color_channel;
    ColorType color_type;
    bool swap_base_detail;
    int numChildren;
    ObjectDataChild* children;
} ObjectData;

ObjectData* getObjectData(const char* filename);

char* isPortalId(int id);