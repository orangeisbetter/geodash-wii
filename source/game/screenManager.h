#pragma once

#include <gccore.h>

// typedef enum {
//     HOME,
//     MENU,
//     LEVEL,
//     SPRITE_SELECTION
// } gameState;

// typedef struct {
//     bool fadeIn;
//     bool fadeOut;
//     int currentScreen;
//     int targetScreen;
//     void* data;
// } ScreenManager;

// typedef struct {
//     int (*load)(void* data);
//     void (*run)(u32 buttons);
// } ScreenContext;

typedef struct {
    void (*enter)(void);
    void (*exit)(void);
    void (*run)(f32 deltaTime);
    void (*render)(void);
} GameState;

void changeState(const GameState* newState, f32 fadeTime);
void runState(f32 deltaTime);

// void runScreen(u32 buttons);
// void changeScreen(int screen, void* data, int fadeInTime, int fadeOutTime);
// void updateScreen();