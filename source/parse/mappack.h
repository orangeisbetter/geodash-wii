#pragma once

typedef struct {
    char* name;
    int* ids;
    int numLevels;
} MapPack;

int mappacksLoad(const char* dirpath);
void mappacksFree();

const MapPack* getMappack(const char* name);