#pragma once

typedef struct {
    const char* name;
    const int* ids;
    int numLevels;
} MapPack;

int mappacksLoad(const char* dirpath);
void mappacksFree();

const MapPack* getMappack(const char* name);