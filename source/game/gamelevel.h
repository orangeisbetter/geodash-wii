#pragma once

#include "../parse/object.h"

extern ObjectData* objectDefaults;

int loadLevel(int levelId);
void runLevel(Mtx view, Mtx modelView, u32 pressed);