#include "physics.h"

const float playerGravity = 0.958199f;
const float basePlayerSpeed = 5.770002f;
const float playerSize = 30.0f;
const float miniModeJumpScale = 0.8f;
const float cubeJumpVel = 11.180032f;

const float jumpBases[SPEED_COUNT] = {
    [SPEED_ONE] = 11.180032f,
    [SPEED_HALF] = 10.62f,
    [SPEED_TWO] = 11.42f,
    [SPEED_THREE] = 11.23f,
    [SPEED_FOUR] = 11.23f
};

const float ballGravityScale = 0.6f;
const float ballSwitchVel = -1.5f;
const float ballMaxVel = 20.0f;

// Horizontal speed in units per second, indexed by Speed (level setting kA4 / speed portals)
const f32 playerSpeeds[SPEED_COUNT] = {
    [SPEED_ONE] = 311.58f / 54,
    [SPEED_HALF] = 251.16f / 54,
    [SPEED_TWO] = 387.42f / 54,
    [SPEED_THREE] = 468.0f / 54,
    [SPEED_FOUR] = 576.0f / 54
};

// Ball spin in degrees per unit travelled (measured from the original game)
const f32 ballSpinPerUnit = 1.85f;

// Full-size values. Mini scaling is handled separately by playerBumpScale().
// Pads are speed-independent; orbs scale with the speed's jump base.
// Blue entries are magnitudes toward the new gravity direction.
const f32 velPads[GAMEMODE_COUNT][SPEED_COUNT][PAD_COUNT] = {
    [GAMEMODE_CUBE] = {
        [SPEED_ONE] = { [PAD_YELLOW] = 16.0f, [PAD_PINK] = 10.4f, [PAD_RED] = 20.0f, [PAD_BLUE] = 6.4f },
        [SPEED_HALF] = { [PAD_YELLOW] = 16.0f, [PAD_PINK] = 10.4f, [PAD_RED] = 20.0f, [PAD_BLUE] = 6.4f },
        [SPEED_TWO] = { [PAD_YELLOW] = 16.0f, [PAD_PINK] = 10.4f, [PAD_RED] = 20.0f, [PAD_BLUE] = 6.4f },
        [SPEED_THREE] = { [PAD_YELLOW] = 16.0f, [PAD_PINK] = 10.4f, [PAD_RED] = 20.0f, [PAD_BLUE] = 6.4f },
        [SPEED_FOUR] = { [PAD_YELLOW] = 16.0f, [PAD_PINK] = 10.4f, [PAD_RED] = 20.0f, [PAD_BLUE] = 6.4f },
    },
    [GAMEMODE_SHIP] = {
        [SPEED_ONE] = { [PAD_YELLOW] = 16.0f, [PAD_PINK] = 5.6f, [PAD_RED] = 10.08f, [PAD_BLUE] = 6.4f },
        [SPEED_HALF] = { [PAD_YELLOW] = 16.0f, [PAD_PINK] = 5.6f, [PAD_RED] = 10.08f, [PAD_BLUE] = 6.4f },
        [SPEED_TWO] = { [PAD_YELLOW] = 16.0f, [PAD_PINK] = 5.6f, [PAD_RED] = 10.08f, [PAD_BLUE] = 6.4f },
        [SPEED_THREE] = { [PAD_YELLOW] = 16.0f, [PAD_PINK] = 5.6f, [PAD_RED] = 10.08f, [PAD_BLUE] = 6.4f },
        [SPEED_FOUR] = { [PAD_YELLOW] = 16.0f, [PAD_PINK] = 5.6f, [PAD_RED] = 10.08f, [PAD_BLUE] = 6.4f },
    },
    [GAMEMODE_BALL] = {
        [SPEED_ONE] = { [PAD_YELLOW] = 9.6f, [PAD_PINK] = 6.72f, [PAD_RED] = 12.0f, [PAD_BLUE] = 3.84f },
        [SPEED_HALF] = { [PAD_YELLOW] = 9.6f, [PAD_PINK] = 6.72f, [PAD_RED] = 12.0f, [PAD_BLUE] = 3.84f },
        [SPEED_TWO] = { [PAD_YELLOW] = 9.6f, [PAD_PINK] = 6.72f, [PAD_RED] = 12.0f, [PAD_BLUE] = 3.84f },
        [SPEED_THREE] = { [PAD_YELLOW] = 9.6f, [PAD_PINK] = 6.72f, [PAD_RED] = 12.0f, [PAD_BLUE] = 3.84f },
        [SPEED_FOUR] = { [PAD_YELLOW] = 9.6f, [PAD_PINK] = 6.72f, [PAD_RED] = 12.0f, [PAD_BLUE] = 3.84f },
    },
    [GAMEMODE_UFO] = {
        [SPEED_ONE] = { [PAD_YELLOW] = 16.0f, [PAD_PINK] = 6.4f, [PAD_RED] = 9.6f, [PAD_BLUE] = 6.4f },
        [SPEED_HALF] = { [PAD_YELLOW] = 16.0f, [PAD_PINK] = 6.4f, [PAD_RED] = 9.6f, [PAD_BLUE] = 6.4f },
        [SPEED_TWO] = { [PAD_YELLOW] = 16.0f, [PAD_PINK] = 6.4f, [PAD_RED] = 9.6f, [PAD_BLUE] = 6.4f },
        [SPEED_THREE] = { [PAD_YELLOW] = 16.0f, [PAD_PINK] = 6.4f, [PAD_RED] = 9.6f, [PAD_BLUE] = 6.4f },
        [SPEED_FOUR] = { [PAD_YELLOW] = 16.0f, [PAD_PINK] = 6.4f, [PAD_RED] = 9.6f, [PAD_BLUE] = 6.4f },
    },
    [GAMEMODE_WAVE] = {
        [SPEED_ONE] = { [PAD_YELLOW] = 16.0f, [PAD_PINK] = 10.4f, [PAD_RED] = 20.0f, [PAD_BLUE] = 6.4f },
        [SPEED_HALF] = { [PAD_YELLOW] = 16.0f, [PAD_PINK] = 10.4f, [PAD_RED] = 20.0f, [PAD_BLUE] = 6.4f },
        [SPEED_TWO] = { [PAD_YELLOW] = 16.0f, [PAD_PINK] = 10.4f, [PAD_RED] = 20.0f, [PAD_BLUE] = 6.4f },
        [SPEED_THREE] = { [PAD_YELLOW] = 16.0f, [PAD_PINK] = 10.4f, [PAD_RED] = 20.0f, [PAD_BLUE] = 6.4f },
        [SPEED_FOUR] = { [PAD_YELLOW] = 16.0f, [PAD_PINK] = 10.4f, [PAD_RED] = 20.0f, [PAD_BLUE] = 6.4f },
    },
    [GAMEMODE_ROBOT] = {
        [SPEED_ONE] = { [PAD_YELLOW] = 16.0f, [PAD_PINK] = 10.4f, [PAD_RED] = 20.0f, [PAD_BLUE] = 6.4f },
        [SPEED_HALF] = { [PAD_YELLOW] = 16.0f, [PAD_PINK] = 10.4f, [PAD_RED] = 20.0f, [PAD_BLUE] = 6.4f },
        [SPEED_TWO] = { [PAD_YELLOW] = 16.0f, [PAD_PINK] = 10.4f, [PAD_RED] = 20.0f, [PAD_BLUE] = 6.4f },
        [SPEED_THREE] = { [PAD_YELLOW] = 16.0f, [PAD_PINK] = 10.4f, [PAD_RED] = 20.0f, [PAD_BLUE] = 6.4f },
        [SPEED_FOUR] = { [PAD_YELLOW] = 16.0f, [PAD_PINK] = 10.4f, [PAD_RED] = 20.0f, [PAD_BLUE] = 6.4f },
    },
    [GAMEMODE_SPIDER] = {
        [SPEED_ONE] = { [PAD_YELLOW] = 9.6f, [PAD_PINK] = 6.72f, [PAD_RED] = 12.0f, [PAD_BLUE] = 3.84f },
        [SPEED_HALF] = { [PAD_YELLOW] = 9.6f, [PAD_PINK] = 6.72f, [PAD_RED] = 12.0f, [PAD_BLUE] = 3.84f },
        [SPEED_TWO] = { [PAD_YELLOW] = 9.6f, [PAD_PINK] = 6.72f, [PAD_RED] = 12.0f, [PAD_BLUE] = 3.84f },
        [SPEED_THREE] = { [PAD_YELLOW] = 9.6f, [PAD_PINK] = 6.72f, [PAD_RED] = 12.0f, [PAD_BLUE] = 3.84f },
        [SPEED_FOUR] = { [PAD_YELLOW] = 9.6f, [PAD_PINK] = 6.72f, [PAD_RED] = 12.0f, [PAD_BLUE] = 3.84f },
    },
    [GAMEMODE_SWING] = {
        [SPEED_ONE] = { [PAD_YELLOW] = 9.6f, [PAD_PINK] = 6.24f, [PAD_RED] = 12.0f, [PAD_BLUE] = 3.84f },
        [SPEED_HALF] = { [PAD_YELLOW] = 9.6f, [PAD_PINK] = 6.24f, [PAD_RED] = 12.0f, [PAD_BLUE] = 3.84f },
        [SPEED_TWO] = { [PAD_YELLOW] = 9.6f, [PAD_PINK] = 6.24f, [PAD_RED] = 12.0f, [PAD_BLUE] = 3.84f },
        [SPEED_THREE] = { [PAD_YELLOW] = 9.6f, [PAD_PINK] = 6.24f, [PAD_RED] = 12.0f, [PAD_BLUE] = 3.84f },
        [SPEED_FOUR] = { [PAD_YELLOW] = 9.6f, [PAD_PINK] = 6.24f, [PAD_RED] = 12.0f, [PAD_BLUE] = 3.84f },
    },
};

// ORB_BLACK is a fixed drop impulse and does not scale with speed.
const f32 velOrbs[GAMEMODE_COUNT][SPEED_COUNT][ORB_COUNT] = {
    [GAMEMODE_CUBE] = {
        [SPEED_ONE] = { [ORB_YELLOW] = 11.18f, [ORB_PINK] = 8.0496f, [ORB_RED] = 15.4284f, [ORB_BLUE] = 4.472f, [ORB_GREEN] = 11.18f, [ORB_BLACK] = 15.0f },
        [SPEED_HALF] = { [ORB_YELLOW] = 10.62f, [ORB_PINK] = 7.6464f, [ORB_RED] = 14.6556f, [ORB_BLUE] = 4.248f, [ORB_GREEN] = 10.62f, [ORB_BLACK] = 15.0f },
        [SPEED_TWO] = { [ORB_YELLOW] = 11.42f, [ORB_PINK] = 8.2224f, [ORB_RED] = 15.7596f, [ORB_BLUE] = 4.568f, [ORB_GREEN] = 11.42f, [ORB_BLACK] = 15.0f },
        [SPEED_THREE] = { [ORB_YELLOW] = 11.23f, [ORB_PINK] = 8.0856f, [ORB_RED] = 15.4974f, [ORB_BLUE] = 4.492f, [ORB_GREEN] = 11.23f, [ORB_BLACK] = 15.0f },
        [SPEED_FOUR] = { [ORB_YELLOW] = 11.23f, [ORB_PINK] = 8.0856f, [ORB_RED] = 15.4974f, [ORB_BLUE] = 4.492f, [ORB_GREEN] = 11.23f, [ORB_BLACK] = 15.0f },
    },
    [GAMEMODE_SHIP] = {
        [SPEED_ONE] = { [ORB_YELLOW] = 11.18f, [ORB_PINK] = 4.1366f, [ORB_RED] = 11.18f, [ORB_BLUE] = 4.472f, [ORB_GREEN] = 7.826f, [ORB_BLACK] = 14.0f },
        [SPEED_HALF] = { [ORB_YELLOW] = 10.62f, [ORB_PINK] = 3.9294f, [ORB_RED] = 10.62f, [ORB_BLUE] = 4.248f, [ORB_GREEN] = 7.434f, [ORB_BLACK] = 14.0f },
        [SPEED_TWO] = { [ORB_YELLOW] = 11.42f, [ORB_PINK] = 4.2254f, [ORB_RED] = 11.42f, [ORB_BLUE] = 4.568f, [ORB_GREEN] = 7.994f, [ORB_BLACK] = 14.0f },
        [SPEED_THREE] = { [ORB_YELLOW] = 11.23f, [ORB_PINK] = 4.1551f, [ORB_RED] = 11.23f, [ORB_BLUE] = 4.492f, [ORB_GREEN] = 7.861f, [ORB_BLACK] = 14.0f },
        [SPEED_FOUR] = { [ORB_YELLOW] = 11.23f, [ORB_PINK] = 4.1551f, [ORB_RED] = 11.23f, [ORB_BLUE] = 4.492f, [ORB_GREEN] = 7.861f, [ORB_BLACK] = 14.0f },
    },
    [GAMEMODE_BALL] = {
        [SPEED_ONE] = { [ORB_YELLOW] = 7.826f, [ORB_PINK] = 6.02602f, [ORB_RED] = 10.48684f, [ORB_BLUE] = 3.1304f, [ORB_GREEN] = 7.826f, [ORB_BLACK] = 15.0f },
        [SPEED_HALF] = { [ORB_YELLOW] = 7.434f, [ORB_PINK] = 5.72418f, [ORB_RED] = 9.96156f, [ORB_BLUE] = 2.9736f, [ORB_GREEN] = 7.434f, [ORB_BLACK] = 15.0f },
        [SPEED_TWO] = { [ORB_YELLOW] = 7.994f, [ORB_PINK] = 6.15538f, [ORB_RED] = 10.71196f, [ORB_BLUE] = 3.1976f, [ORB_GREEN] = 7.994f, [ORB_BLACK] = 15.0f },
        [SPEED_THREE] = { [ORB_YELLOW] = 7.861f, [ORB_PINK] = 6.05297f, [ORB_RED] = 10.53374f, [ORB_BLUE] = 3.1444f, [ORB_GREEN] = 7.861f, [ORB_BLACK] = 15.0f },
        [SPEED_FOUR] = { [ORB_YELLOW] = 7.861f, [ORB_PINK] = 6.05297f, [ORB_RED] = 10.53374f, [ORB_BLUE] = 3.1444f, [ORB_GREEN] = 7.861f, [ORB_BLACK] = 15.0f },
    },
    [GAMEMODE_UFO] = {
        [SPEED_ONE] = { [ORB_YELLOW] = 11.18f, [ORB_PINK] = 4.6956f, [ORB_RED] = 11.4036f, [ORB_BLUE] = 4.472f, [ORB_GREEN] = 11.18f, [ORB_BLACK] = 11.2f },
        [SPEED_HALF] = { [ORB_YELLOW] = 10.62f, [ORB_PINK] = 4.4604f, [ORB_RED] = 10.8324f, [ORB_BLUE] = 4.248f, [ORB_GREEN] = 10.62f, [ORB_BLACK] = 11.2f },
        [SPEED_TWO] = { [ORB_YELLOW] = 11.42f, [ORB_PINK] = 4.7964f, [ORB_RED] = 11.6484f, [ORB_BLUE] = 4.568f, [ORB_GREEN] = 11.42f, [ORB_BLACK] = 11.2f },
        [SPEED_THREE] = { [ORB_YELLOW] = 11.23f, [ORB_PINK] = 4.7166f, [ORB_RED] = 11.4546f, [ORB_BLUE] = 4.492f, [ORB_GREEN] = 11.23f, [ORB_BLACK] = 11.2f },
        [SPEED_FOUR] = { [ORB_YELLOW] = 11.23f, [ORB_PINK] = 4.7166f, [ORB_RED] = 11.4546f, [ORB_BLUE] = 4.492f, [ORB_GREEN] = 11.23f, [ORB_BLACK] = 11.2f },
    },
    [GAMEMODE_WAVE] = {
        [SPEED_ONE] = { [ORB_YELLOW] = 11.18f, [ORB_PINK] = 8.0496f, [ORB_RED] = 15.4284f, [ORB_BLUE] = 4.472f, [ORB_GREEN] = 11.18f, [ORB_BLACK] = 14.0f },
        [SPEED_HALF] = { [ORB_YELLOW] = 10.62f, [ORB_PINK] = 7.6464f, [ORB_RED] = 14.6556f, [ORB_BLUE] = 4.248f, [ORB_GREEN] = 10.62f, [ORB_BLACK] = 14.0f },
        [SPEED_TWO] = { [ORB_YELLOW] = 11.42f, [ORB_PINK] = 8.2224f, [ORB_RED] = 15.7596f, [ORB_BLUE] = 4.568f, [ORB_GREEN] = 11.42f, [ORB_BLACK] = 14.0f },
        [SPEED_THREE] = { [ORB_YELLOW] = 11.23f, [ORB_PINK] = 8.0856f, [ORB_RED] = 15.4974f, [ORB_BLUE] = 4.492f, [ORB_GREEN] = 11.23f, [ORB_BLACK] = 14.0f },
        [SPEED_FOUR] = { [ORB_YELLOW] = 11.23f, [ORB_PINK] = 8.0856f, [ORB_RED] = 15.4974f, [ORB_BLUE] = 4.492f, [ORB_GREEN] = 11.23f, [ORB_BLACK] = 14.0f },
    },
    [GAMEMODE_ROBOT] = {
        [SPEED_ONE] = { [ORB_YELLOW] = 10.062f, [ORB_PINK] = 8.0496f, [ORB_RED] = 14.3104f, [ORB_BLUE] = 4.472f, [ORB_GREEN] = 11.18f, [ORB_BLACK] = 15.0f },
        [SPEED_HALF] = { [ORB_YELLOW] = 9.558f, [ORB_PINK] = 7.6464f, [ORB_RED] = 13.5936f, [ORB_BLUE] = 4.248f, [ORB_GREEN] = 10.62f, [ORB_BLACK] = 15.0f },
        [SPEED_TWO] = { [ORB_YELLOW] = 10.278f, [ORB_PINK] = 8.2224f, [ORB_RED] = 14.6176f, [ORB_BLUE] = 4.568f, [ORB_GREEN] = 11.42f, [ORB_BLACK] = 15.0f },
        [SPEED_THREE] = { [ORB_YELLOW] = 10.107f, [ORB_PINK] = 8.0856f, [ORB_RED] = 14.3744f, [ORB_BLUE] = 4.492f, [ORB_GREEN] = 11.23f, [ORB_BLACK] = 15.0f },
        [SPEED_FOUR] = { [ORB_YELLOW] = 10.107f, [ORB_PINK] = 8.0856f, [ORB_RED] = 14.3744f, [ORB_BLUE] = 4.492f, [ORB_GREEN] = 11.23f, [ORB_BLACK] = 15.0f },
    },
    [GAMEMODE_SPIDER] = {
        [SPEED_ONE] = { [ORB_YELLOW] = 7.826f, [ORB_PINK] = 5.63472f, [ORB_RED] = 10.48684f, [ORB_BLUE] = 3.1304f, [ORB_GREEN] = 7.826f, [ORB_BLACK] = 16.5f },
        [SPEED_HALF] = { [ORB_YELLOW] = 7.434f, [ORB_PINK] = 5.35248f, [ORB_RED] = 9.96156f, [ORB_BLUE] = 2.9736f, [ORB_GREEN] = 7.434f, [ORB_BLACK] = 16.5f },
        [SPEED_TWO] = { [ORB_YELLOW] = 7.994f, [ORB_PINK] = 5.75568f, [ORB_RED] = 10.71196f, [ORB_BLUE] = 3.1976f, [ORB_GREEN] = 7.994f, [ORB_BLACK] = 16.5f },
        [SPEED_THREE] = { [ORB_YELLOW] = 7.861f, [ORB_PINK] = 5.65992f, [ORB_RED] = 10.53374f, [ORB_BLUE] = 3.1444f, [ORB_GREEN] = 7.861f, [ORB_BLACK] = 16.5f },
        [SPEED_FOUR] = { [ORB_YELLOW] = 7.861f, [ORB_PINK] = 5.65992f, [ORB_RED] = 10.53374f, [ORB_BLUE] = 3.1444f, [ORB_GREEN] = 7.861f, [ORB_BLACK] = 16.5f },
    },
    [GAMEMODE_SWING] = {
        [SPEED_ONE] = { [ORB_YELLOW] = 6.708f, [ORB_PINK] = 4.82976f, [ORB_RED] = 9.25704f, [ORB_BLUE] = 2.6832f, [ORB_GREEN] = 6.708f, [ORB_BLACK] = 14.0f },
        [SPEED_HALF] = { [ORB_YELLOW] = 6.372f, [ORB_PINK] = 4.58784f, [ORB_RED] = 8.79336f, [ORB_BLUE] = 2.5488f, [ORB_GREEN] = 6.372f, [ORB_BLACK] = 14.0f },
        [SPEED_TWO] = { [ORB_YELLOW] = 6.852f, [ORB_PINK] = 4.93344f, [ORB_RED] = 9.45576f, [ORB_BLUE] = 2.7408f, [ORB_GREEN] = 6.852f, [ORB_BLACK] = 14.0f },
        [SPEED_THREE] = { [ORB_YELLOW] = 6.738f, [ORB_PINK] = 4.85136f, [ORB_RED] = 9.29844f, [ORB_BLUE] = 2.6952f, [ORB_GREEN] = 6.738f, [ORB_BLACK] = 14.0f },
        [SPEED_FOUR] = { [ORB_YELLOW] = 6.738f, [ORB_PINK] = 4.85136f, [ORB_RED] = 9.29844f, [ORB_BLUE] = 2.6952f, [ORB_GREEN] = 6.738f, [ORB_BLACK] = 14.0f },
    },
};

