#pragma once

typedef struct {
    int id;
    const char* name;
    const char* music;
} LevelInfo;

typedef struct {
    LevelInfo* levels;
    int numLevels;
} LevelsInfo;

extern LevelsInfo levelsInfo;
extern int mainLevels[8];

LevelInfo* getLevelInfoById(int id);