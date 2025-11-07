#include "object.h"

#include <stdio.h>
#include <cJSON.h>
#include <string.h>
#include <malloc.h>
#include <gccore.h>
#include <stdlib.h>

#define JSON_BOOL(obj, key, target)                                  \
    do {                                                             \
        cJSON* tmp = cJSON_GetObjectItemCaseSensitive((obj), (key)); \
        if (cJSON_IsBool(tmp))                                       \
            (target) = cJSON_IsTrue(tmp);                            \
    } while (0)

#define JSON_STRING(obj, key, target)                                \
    do {                                                             \
        cJSON* tmp = cJSON_GetObjectItemCaseSensitive((obj), (key)); \
        if (cJSON_IsString(tmp) && tmp->valuestring)                 \
            (target) = strdup(tmp->valuestring);                     \
    } while (0)

#define JSON_INT(obj, key, target)                                   \
    do {                                                             \
        cJSON* tmp = cJSON_GetObjectItemCaseSensitive((obj), (key)); \
        if (cJSON_IsNumber(tmp))                                     \
            (target) = tmp->valueint;                                \
    } while (0)

inline void JSON_ColorType(cJSON* obj, ColorType* target) {
    cJSON* color_type = cJSON_GetObjectItemCaseSensitive((obj), "color_type");
    if (!cJSON_IsString(color_type)) {
        return;
    }
    if (strcmp(color_type->valuestring, "Detail") == 0) {
        *target = COLOR_TYPE_DETAIL;
    } else if (strcmp(color_type->valuestring, "Base") == 0) {
        *target = COLOR_TYPE_BASE;
    } else if (strcmp(color_type->valuestring, "Black") == 0) {
        *target = COLOR_TYPE_BLACK;
    } else {
        //*target = COLOR_TYPE_BASE;
        SYS_Report("Unknown color type %s\n", color_type->valuestring);
    }
}

#define JSON_FLOAT(obj, key, target)                                 \
    do {                                                             \
        cJSON* tmp = cJSON_GetObjectItemCaseSensitive((obj), (key)); \
        if (cJSON_IsNumber(tmp)) {                                   \
            (target) = (float)(tmp->valuedouble);                    \
        }                                                            \
    } while (0)

int streq(const char* a, const char* b) {
    while (*a == *b) {
        if (!*a) {
            return 1;
        }
        a++;
        b++;
    }
    return 0;
}

ObjectData* getObjectData(const char* filename) {
    // read json file into character buffer
    FILE* objectsFile = fopen(filename, "rb");
    fseek(objectsFile, 0, SEEK_END);
    long numberOfCharactersInFile = ftell(objectsFile);
    char* objectBuffer = malloc(numberOfCharactersInFile * sizeof(char));
    fseek(objectsFile, 0, SEEK_SET);
    fread(objectBuffer, numberOfCharactersInFile, sizeof(char), objectsFile);
    fclose(objectsFile);

    cJSON* objectsJson = cJSON_ParseWithLength(objectBuffer, numberOfCharactersInFile);
    free(objectBuffer);
    if (objectsJson == NULL) {
        SYS_Report("OBJECTS ARE NULL!!!!\n");
    }

    long arraySize = 0;

    for (cJSON* item = objectsJson->child; item != NULL; item = item->next) {
        long id = strtol(item->string, NULL, 10);
        if (id > arraySize) {
            arraySize = id;
        }
    }
    arraySize++;

    ObjectData* objects = calloc(arraySize, sizeof(ObjectData));

    for (cJSON* item = objectsJson->child; item != NULL; item = item->next) {
        long id = strtol(item->string, NULL, 10);
        ObjectData* object = &objects[id];
        object->exists = true;

        JSON_STRING(item, "texture", object->texture);
        JSON_INT(item, "default_z_layer", object->default_z_layer);
        JSON_INT(item, "default_z_order", object->default_z_order);
        JSON_INT(item, "default_base_color_channel", object->default_base_color_channel);
        JSON_INT(item, "default_detail_color_channel", object->default_detail_color_channel);
        JSON_BOOL(item, "swap_base_detail", object->swap_base_detail);
        JSON_ColorType(item, &object->color_type);

        cJSON* hitbox = cJSON_GetObjectItem(item, "hitbox");
        if (!cJSON_IsNull(hitbox)) {
            cJSON* type = cJSON_GetObjectItem(hitbox, "type");
            const char* sv;
            if (cJSON_IsString(type)) {
                const char* sv = cJSON_GetStringValue(type);
                if (streq(sv, "Box")) {
                    object->hitbox.type = HITBOX_TYPE_BOX;
                } else {
                    object->hitbox.type = HITBOX_TYPE_NONE;
                }
            } else {
                object->hitbox.type = HITBOX_TYPE_NONE;
            }

            JSON_FLOAT(hitbox, "x", object->hitbox.x);
            JSON_FLOAT(hitbox, "y", object->hitbox.y);
            JSON_FLOAT(hitbox, "width", object->hitbox.width);
            JSON_FLOAT(hitbox, "height", object->hitbox.height);
        } else {
            object->hitbox.type = HITBOX_TYPE_NONE;
        }

        cJSON* childrenJson = cJSON_GetObjectItemCaseSensitive(item, "children");
        if (cJSON_IsArray(childrenJson)) {
            int numberOfChildren = cJSON_GetArraySize(childrenJson);
            ObjectDataChild* children = malloc(numberOfChildren * sizeof(ObjectDataChild));
            object->numChildren = numberOfChildren;
            object->children = children;

            for (int i = 0; i < numberOfChildren; i++) {
                cJSON* childJson = cJSON_GetArrayItem(childrenJson, i);
                ObjectDataChild* child = &children[i];

                JSON_STRING(childJson, "texture", child->texture);
                JSON_FLOAT(childJson, "anchor_x", child->anchor_x);
                JSON_FLOAT(childJson, "anchor_y", child->anchor_y);
                JSON_FLOAT(childJson, "scale_x", child->scale_x);
                JSON_FLOAT(childJson, "scale_y", child->scale_y);
                JSON_BOOL(childJson, "flip_x", child->flip_x);
                JSON_BOOL(childJson, "flip_y", child->flip_y);
                JSON_FLOAT(childJson, "x", child->x);
                JSON_FLOAT(childJson, "y", child->y);
                JSON_INT(childJson, "z", child->z);
                JSON_FLOAT(childJson, "rot", child->rot);
                JSON_ColorType(childJson, &child->color_type);
            }
        }
    }

    cJSON_free(objectsJson);

    // for (int i = 0; i < arraySize; i++) {
    //     if (objects[i].exists == true) {
    //         SYS_Report("Texture for id %d: %s\n", i, objects[i].texture);
    //         // SYS_Report("  default_z_layer: %u\n", objects[i].default_z_layer);
    //         // SYS_Report("  default_z_order: %d\n", objects[i].default_z_order);
    //         // SYS_Report("  default_base_color_channel: %d\n", objects[i].default_base_color_channel);
    //         // SYS_Report("  default_detail_color_channel: %d\n", objects[i].default_detail_color_channel);
    //         // SYS_Report("  color_type: %d\n", (int)objects[i].color_type);  // Adjust if you want enum names
    //         // SYS_Report("  swap_base_detail: %s\n", objects[i].swap_base_detail ? "true" : "false");
    //         SYS_Report("  children:\n");
    //         for (int j = 0; j < objects[i].numChildren; j++) {
    //             ObjectDataChild* child = &objects[i].children[j];
    //             SYS_Report("    texture: %s\n", child->texture);
    //             SYS_Report("    anchor_x: %f\n", child->anchor_x);
    //             SYS_Report("    anchor_y: %f\n", child->anchor_y);
    //             SYS_Report("    scale_x: %f\n", child->scale_x);
    //             SYS_Report("    scale_y: %f\n", child->scale_y);
    //             SYS_Report("    flip_x: %s\n", child->flip_x ? "true" : "false");
    //             SYS_Report("    flip_y: %s\n", child->flip_y ? "true" : "false");
    //             SYS_Report("    x: %f\n", child->x);
    //             SYS_Report("    y: %f\n", child->y);
    //             SYS_Report("    z: %d\n", child->z);
    //             SYS_Report("    rot: %f\n", child->rot);
    //             SYS_Report("    color_type: %d\n", (int)child->color_type);
    //         }
    //     }
    // }

    return objects;
}

char* isPortalId(int id) {
    switch (id) {
        case 10:
            return "portal_01_back_001.png";
        case 11:
            return "portal_02_back_001.png";
        case 12:
            return "portal_03_back_001.png";
        case 13:
            return "portal_04_back_001.png";
        case 45:
            return "portal_05_back_001.png";
        case 46:
            return "portal_06_back_001.png";
        case 47:
            return "portal_07_back_001.png";
        case 99:
            return "portal_08_back_001.png";
        case 101:
            return "portal_09_back_001.png";
        case 111:
            return "portal_10_back_001.png";
        case 286:
            return "portal_11_back_001.png";
        case 287:
            return "portal_12_back_001.png";
        case 660:
            return "portal_13_back_001.png";
        case 745:
            return "portal_14_back_001.png";
        case 747:
            return "portal_15_back_001.png";
        case 749:
            return "portal_16_back_001.png";
        case 1331:
            return "portal_17_back_001.png";
        default:
            return NULL;
    }
}