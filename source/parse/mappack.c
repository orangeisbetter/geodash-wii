#include "mappack.h"

#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>

#include <fat.h>
#include <dirent.h>
#include <gccore.h>

#include "../vendor/xml.h"

static MapPack* mappacks = NULL;
static int size = 0;
static int capacity = 0;

static int mappackLoad(const char* filename, MapPack* mappack) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        return 0;
    }

    struct xml_document* document = xml_open_document(file);
    fclose(file);
    if (!document) {
        return 0;
    }

    struct xml_node* root = xml_document_root(document);
    mappack->numLevels = xml_node_children(root);

    // mappack name
    struct xml_string* str = xml_node_attribute_content(root, 0);
    int len = xml_string_length(str);
    char* name = malloc(sizeof(char) * (len + 1));
    if (!name) {
        return 0;
    }
    xml_string_copy(str, (uint8_t*)name, len);
    name[len] = '\0';

    mappack->name = name;

    // level ids
    int* ids = malloc(sizeof(*ids) * mappack->numLevels);
    if (!ids) {
        free(name);
        return 0;
    }

    for (int i = 0; i < mappack->numLevels; i++) {
        struct xml_node* level = xml_node_child(root, i);

        str = xml_node_content(level);
        char buf[10];
        len = xml_string_length(str);
        xml_string_copy(str, (uint8_t*)buf, 9);
        buf[len < 10 ? len : 9] = '\0';
        ids[i] = atoi(buf);
        if (ids[i] == 0) {
            free(name);
            free(ids);
            return 0;
        }
    }

    mappack->ids = ids;

    SYS_Report("Loaded mappack \"%s\": %d levels\n", mappack->name, mappack->numLevels);

    xml_document_free(document, true);
    return 1;
}

int mappacksLoad(const char* dirpath) {
    DIR* directory = opendir(dirpath);
    if (!directory) {
        return 0;
    }

    struct dirent* entry;
    while ((entry = readdir(directory)) != NULL) {
        if (entry->d_type == DT_DIR) {
            continue;
        }
        const char* dot = strrchr(entry->d_name, '.');
        if (!dot || strcasecmp(dot, ".xml")) {
            continue;
        }

        if (capacity == 0) {
            size = 0;
            MapPack* newMappacks = malloc(sizeof(MapPack) * 10);
            if (!newMappacks) {
                mappacksFree();
                capacity = 0;
                return 0;
            }
            mappacks = newMappacks;
            capacity = 10;
        }

        if (size == capacity) {
            int newCapacity = 2 * capacity;
            MapPack* newMappacks = realloc(mappacks, sizeof(MapPack) * newCapacity);
            if (!newMappacks) {
                mappacksFree();
                capacity = 0;
                size = 0;
                return 0;
            }
            mappacks = newMappacks;
            capacity = newCapacity;
        }

        MapPack* newMappack = &mappacks[size];

        char filepath[PATH_MAX + 1];
        snprintf(filepath, PATH_MAX + 1, "%s/%s", dirpath, entry->d_name);
        if (!mappackLoad(filepath, newMappack)) {
            SYS_Report("Unable to open mappack %s\n", entry->d_name);
            continue;
        }

        size++;
    }

    closedir(directory);

    return 1;
}

void mappacksFree() {
    for (int i = 0; i < size; i++) {
        free((char*)mappacks[i].name);
        free((int*)mappacks[i].ids);
    }
    free(mappacks);
    size = 0;
}

const MapPack* getMappack(const char* name) {
    for (int i = 0; i < size; i++) {
        if (strcasecmp(name, mappacks[i].name) == 0) {
            return &mappacks[i];
        }
    }
    return NULL;
}
