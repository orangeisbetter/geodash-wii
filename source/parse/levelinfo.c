#include "levelinfo.h"

#include <stddef.h>

static LevelInfo levels[] = {
    { 1, "Stereo Madness", "StereoMadness.mp3" },
    { 2, "Back On Track", "BackOnTrack.mp3" },
    { 3, "Polargeist", "Polargeist.mp3" },
    { 4, "Dry Out", "DryOut.mp3" },
    { 5, "Base After Base", "BaseAfterBase.mp3" },
    { 6, "Can't Let Go", "CantLetGo.mp3" },
    { 7, "Jumper", "Jumper.mp3" },
    { 8, "Time Machine", "TimeMachine.mp3" },
};

LevelsInfo levelsInfo = {
    .levels = levels,
    .numLevels = sizeof(levels) / sizeof(*levels)
};

int mainLevels[] = { 1, 2, 3, 4, 5, 6, 7, 8 };

LevelInfo* getLevelInfoById(int id) {
    for (int i = 0; i < levelsInfo.numLevels; i++) {
        if (levelsInfo.levels[i].id == id) {
            return &levelsInfo.levels[i];
        }
    }
    return NULL;
}
