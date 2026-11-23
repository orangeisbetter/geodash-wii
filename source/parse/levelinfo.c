#include "levelinfo.h"

#include <stddef.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <gccore.h>

#include "../vendor/xml.h"

typedef struct {
    LevelInfo* levels;
    int numLevels;
} LevelsInfo;

static LevelsInfo levelsStore = {
    .levels = NULL,
    .numLevels = 0
};

static int parseLevels(struct xml_node* levelsNode, LevelsInfo* levels) {
    for (int i = 0; i < levels->numLevels; i++) {
        struct xml_node* levelNode = xml_node_child(levelsNode, i);
        LevelInfo* levelInfo = &levels->levels[i];

        // ID
        {
            struct xml_string* str = xml_node_attribute_content(levelNode, 0);
            if (!str) {
                return 0;
            }

            char buf[10];
            int len = xml_string_length(str);
            xml_string_copy(str, (uint8_t*)buf, 10 - 1);
            buf[len < 10 ? len : 10 - 1] = '\0';
            levelInfo->id = atoi(buf);
            if (!levelInfo->id) {
                return 0;
            }
        }

        // name
        {
            struct xml_node* child = xml_node_child(levelNode, 0);
            if (!child) {
                return 0;
            }
            struct xml_string* str = xml_node_content(child);
            int len = xml_string_length(str);
            levelInfo->name = malloc(sizeof(char) * (len + 1));
            if (!levelInfo->name) {
                return 0;
            }
            xml_string_copy(str, (uint8_t*)levelInfo->name, len);
            levelInfo->name[len] = '\0';
        }

        // music
        {
            struct xml_node* child = xml_node_child(levelNode, 1);
            if (!child) {
                return 0;
            }
            struct xml_string* str = xml_node_content(child);
            int len = xml_string_length(str);
            levelInfo->music = malloc(sizeof(char) * (len + 1));
            if (!levelInfo->music) {
                return 0;
            }
            xml_string_copy(str, (uint8_t*)levelInfo->music, len);
            levelInfo->music[len] = '\0';
        }
    }

    return 1;
}

int levelsInit(const char* filepath) {
    FILE* file = fopen(filepath, "r");
    if (!file) {
        return 0;
    }

    struct xml_document* document = xml_open_document(file);
    fclose(file);
    if (!document) {
        return 0;
    }

    struct xml_node* levelsNode = xml_document_root(document);
    int numLevels = xml_node_children(levelsNode);

    LevelInfo* levels = calloc(numLevels, sizeof(LevelInfo));
    if (!levels) {
        return 0;
    }

    LevelsInfo levelsInfo = {
        .levels = levels,
        .numLevels = numLevels
    };

    if (!parseLevels(levelsNode, &levelsInfo)) {
        for (int i = 0; i < numLevels; i++) {
            LevelInfo* level = &levels[i];
            if (level->name) {
                free(level->name);
            }
            if (level->music) {
                free(level->music);
            }
        }
        free(levels);
        numLevels = 0;
        return 0;
    }

    xml_document_free(document, true);

    levelsStore = levelsInfo;
    return 1;
}

void levelsFree() {
    for (int i = 0; i < levelsStore.numLevels; i++) {
        LevelInfo* level = &levelsStore.levels[i];
        if (level->name) {
            free(level->name);
        }
        if (level->music) {
            free(level->music);
        }
    }
    free(levelsStore.levels);
    levelsStore.levels = NULL;
    levelsStore.numLevels = 0;
}

const LevelInfo* levelStoreSearch(int id) {
    for (int i = 0; i < levelsStore.numLevels; i++) {
        if (levelsStore.levels[i].id == id) {
            return &levelsStore.levels[i];
        }
    }
    return NULL;
}
