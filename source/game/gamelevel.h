#pragma once

#include <gccore.h>

#include "screenManager.h"
#include "../parse/object.h"

extern const GameState GAMESTATE_LEVEL;

extern ObjectData* objectDefaults;

extern const char* levelNames[22];

int loadLevel(int levelId);