#pragma once

#include "../parse/level.h"

// Number of speed tiers, including the level's starting speed.
enum { SPEED_COUNT = SPEED_FOUR + 1 };

typedef enum {
    PAD_YELLOW,
    PAD_PINK,
    PAD_RED,
    PAD_BLUE,
    PAD_COUNT,
} PadType;

typedef enum {
    ORB_YELLOW,
    ORB_PINK,
    ORB_RED,
    ORB_BLUE,
    ORB_GREEN,
    ORB_BLACK,
    ORB_COUNT
} OrbType;

// General player physics
extern const float playerGravity;
extern const float basePlayerSpeed;
extern const float playerSize;
extern const float miniModeJumpScale;
extern const float cubeJumpVel;

// Horizontal speed and jump acceleration by Speed
extern const f32 playerSpeeds[SPEED_COUNT];
extern const float jumpBases[SPEED_COUNT];

// Ball physics
extern const float ballGravityScale;
extern const float ballSwitchVel;
extern const float ballMaxVel;
extern const f32 ballSpinPerUnit;

// Full-size pad/orb impulses. Mini scaling is handled separately by
// playerBumpScale(). Pads are speed-independent; orbs scale with jumpBases.
// Blue entries are magnitudes toward the new gravity direction.
extern const f32 velPads[GAMEMODE_COUNT][SPEED_COUNT][PAD_COUNT];
extern const f32 velOrbs[GAMEMODE_COUNT][SPEED_COUNT][ORB_COUNT];
