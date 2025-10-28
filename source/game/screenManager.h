#pragma once

#include <gccore.h>

typedef enum {
    HOME,
    MENU,
    LEVEL,
    SPRITE_SELECTION
} gameState;

typedef struct {
    gameState screenState;
    gameState targetScreenState;
    bool fadeIn;
    bool fadeOut;
} ScreenManager;

extern ScreenManager screenManager;

void changeScreen(gameState state, int fadeInTime, int fadeOutTime);
void updateScreen();