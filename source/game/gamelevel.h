#pragma once

#include <gccore.h>

#include "screenManager.h"
#include "../parse/object.h"

extern const GameState GAMESTATE_LEVEL;

extern ObjectData* objectDefaults;
extern unsigned int objectDefaultsLength;

int loadLevel(int levelId);
void levelRestart();
