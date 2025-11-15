#pragma once

typedef struct {
    int id;
    char* name;
    char* music;
} LevelInfo;

int levelsInit(const char* filepath);
void levelsFree();

const LevelInfo* levelStoreSearch(int id);