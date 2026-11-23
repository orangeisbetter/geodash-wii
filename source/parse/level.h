#pragma once

#include <gccore.h>
#include "object.h"
#include "../color.h"
#include "../game/gamemode.h"

typedef struct {
    float x;
    float y;
    float width;
    float height;
} Rect;

typedef struct {
    int id;
    f32 x;
    f32 y;
    bool flipx;
    bool flipy;
    f32 rotation;
    u8 z_layer;
    s32 z_order;
    bool is_trigger;
    union {
        struct {  // Values only on triggers
            u8 red;
            u8 green;
            u8 blue;
            f32 duration;
            f32 opacity;
            u16 color_channel;
            bool player_color_1;
            bool player_color_2;
            bool blending;
        };
    };
    u16 base_color_channel;
    u16 detail_color_channel;
    Rect hitbox;
} LevelObject;

typedef enum{
    ONE,
    HALF,
    TWO,
    THREE,
    FOUR
} Speed;

typedef struct{
    ColorConfig colorConfig;
    Gamemode gamemode;
    bool minimode;
    Speed speed;
    bool obj2Blending;
    int BGTexId;
    int groundTexId;
    bool dualMode;
    bool startPosObj;
    bool multiPlayerMode;
    bool flipGravity;
    f32 songOffset;
    // GuidelineString guidelineString;
    bool fadeIn;
    bool fadeOut;
    int groundLineId;
    int font;
    bool reverseMode;
    bool platformerMode;
    int middlegroundTexId;
    bool mirrorMode;
    bool rotateGameplay;

} LevelData;

typedef struct {
    int id;
    f32 x;
    f32 y;
    bool flipx;
    bool flipy;
    f32 rotation;
} DrawableObject;

typedef struct {
    int id;
} TriggerObject;

typedef struct {
    f32 anchor_x;
    f32 anchor_y;
    int color_channel;
    f32 content_x;
    f32 content_y;
    bool flipx;
    bool flipy;
    f32 rotation;
    f32 scale_x;
    f32 scale_y;
    const char* texture_name;
} LevelObjectInfo;

typedef struct {
    int gamemode;
    int speed;
    int background;
    int ground;
    int font;
    f32 songOffset;
    bool mini;
    bool dual;
    f32 startPos;
    bool twop;
    bool flipgravity;
    int guidelines;
    f32 songfadein;
    f32 songfadeout;
    bool line;
} LevelSettings;

typedef struct {
    char* key;
    int keyLen;
    char* value;
    int valueLen;
} LevelKV;

typedef struct {
    LevelSettings settings;
    LevelObject* objects;
    int numObjects;
} GDLevel;

char* GDL_OpenDecodedFile(char* filename);
char* GDL_GetLevelString(char* filename);
LevelObject* GDL_ParseLevelString(char* levelString, int* size, ObjectData* dataObjects, LevelData* levelData);

// GDLevel* GDL_parseLevel(char* levelString);