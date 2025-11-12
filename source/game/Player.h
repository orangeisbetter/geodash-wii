#pragma once

#include <stdbool.h>

#include "gamemode.h"

#include "../parse/level.h"

typedef struct {
    float x;
    float y;
    float velX;
    float velY;
    float rotation;
    float gravity;
    float jumpVel;
    float groundBottom;     // Bottom of restricted zone
    float groundTop;        // Top of restricted zone
    float gravityModifier;  // 1 for normal gravity, -1 for flipped gravity
    bool gravityFlipped;
    bool restrictedZone;
    bool grounded;
    bool canJump;
    bool rising;
    bool holding;
    bool queuedHold;
    bool dead;
    Gamemode gamemode;
} Player;

typedef struct {
    float x;
    float y;
    float width;
    float height;
} Rect;

Player* playerInit(Player* player);
bool playerIsFalling(Player* player);
void playerHitGround(Player* player, bool reverseGravity);
void playerDestroy(Player* player);
void playerGetRect(Player* player, Rect* rect);
void playerChangeGamemode(Player* player, Gamemode gamemode);
void playerPadJump(Player* player, float velocity);
void playerRingJump(Player* player, float strength);
void playerCollideWithObject(Player* player, LevelObject* object, ObjectData* class);
void playerUpdate(Player* player, float dt);