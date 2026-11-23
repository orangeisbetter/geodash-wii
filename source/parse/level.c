#include "level.h"

#include <stdio.h>
#include <zlib.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "../base64.h"
#include "../data.h"
#include "../color.h"

#define CHUNK_SIZE 8192

unsigned char* zlib_decompress(unsigned char* input, long inputLength, long* outputLength, long* outputCapacity) {
    int ret;
    z_stream strm;

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = inputLength;
    strm.next_in = input;
    ret = inflateInit2(&strm, MAX_WBITS | 16);
    if (ret != Z_OK) {
        SYS_Report("Error init\n");
        return NULL;
    }

    long outputSize = CHUNK_SIZE;
    unsigned char* output = malloc(outputSize);

    if (output == NULL) {
        SYS_Report("Error allocating output buffer\n");
        inflateEnd(&strm);
        return NULL;
    }

    size_t totalSize = 0;
    do {
        if (totalSize + CHUNK_SIZE > outputSize) {
            outputSize *= 2;
            unsigned char* newOutput = realloc(output, outputSize);
            if (newOutput == NULL) {
                SYS_Report("Error reallocating output buffer\n");
                free(output);
                inflateEnd(&strm);
                return NULL;
            }
            output = newOutput;
        }

        strm.next_out = output + totalSize;
        strm.avail_out = outputSize - totalSize;

        ret = inflate(&strm, Z_NO_FLUSH);
        assert(ret != Z_STREAM_ERROR);
        if (ret == Z_MEM_ERROR || ret == Z_DATA_ERROR || ret == Z_NEED_DICT) {
            SYS_Report("Error inflating: %d, %s\n", ret, strm.msg);
            inflateEnd(&strm);
            return NULL;
        }

        totalSize = outputSize - strm.avail_out;
    } while (ret != Z_STREAM_END);

    inflateEnd(&strm);
    *outputLength = totalSize;
    *outputCapacity = outputSize;
    return output;
}

char* GDL_OpenDecodedFile(char* filename) {
    FILE* file = fopen(filename, "rb");

    if (!file) {
        SYS_Report("Unable to open file %s: %s\n", filename, strerror(errno));
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = malloc(length + 1);
    fread(buffer, 1, length, file);
    buffer[length] = '\0';

    fclose(file);

    return buffer;
}

char* GDL_GetLevelString(char* filename) {
    FILE* file = fopen(filename, "rb");

    if (!file) {
        SYS_Report("Unable to open file %s: %s\n", filename, strerror(errno));
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = malloc(length + 1);
    fread(buffer, 1, length, file);
    buffer[length] = '\0';

    fclose(file);

    long compressedLength = Base64decode_len(buffer);
    unsigned char* compressed = malloc(compressedLength);
    if (compressed == NULL) {
        free(buffer);
        SYS_Report("There was an error decoding base64 (allocating space).\n");
        return NULL;
    }
    Base64decode((char*)compressed, buffer);
    free(buffer);

    long decompressedLength = -1;
    long decompressedCapacity = -1;
    char* decompressed = (char*)zlib_decompress((unsigned char*)compressed, compressedLength, &decompressedLength, &decompressedCapacity);
    free(compressed);
    if (decompressed == NULL) {
        SYS_Report("There was an error decompressing.\n");
        return NULL;
    }
    if (decompressedLength == decompressedCapacity) {
        decompressed = realloc(decompressed, decompressedCapacity + 1);
        if (decompressed == NULL) {
            free(decompressed);
            return NULL;
        }
    }
    decompressed[decompressedLength] = '\0';

    // FILE* dest = fopen("sd:/gdwii/assets/levels/decompressed.txt", "wb");
    // fwrite(decompressed, 1, decompressedLength, dest);
    // fclose(dest);

    return decompressed;
}

/**
 * Gets the next piece of data (key or value) as a string.
 *
 * @param[in] string A pointer to the input string, starting at the first piece of data
 * @param[out] length An out pointer to write the length of the read piece of data to
 * @param[out] nextString An out pointer to write the location of the next piece of data, or NULL if the end of the string is reached.
 * @param[out] lastOfObject An out pointer to write the boolean of if the the data is the last of an object (followed by a `;`)
 *
 * @return A pointer to the data segment (string view)
 */
char* getNextData(char* string, int* length, char** nextString, bool* lastOfObject) {
    char* start = string;
    while (*string != ',' && *string != ';' && *string != '\0') {
        string++;
    }
    *length = string - start;
    *lastOfObject = false;
    switch (*string) {
        case ';':
            *lastOfObject = true;
        case ',':
            string++;
            break;
        case '\0':
            string = NULL;
            break;
    }
    *nextString = string;
    start[*length] = '\0';
    return start;
}

const LevelObject defaultObject = {
    .id = 0,
    .x = 0,
    .y = 0,
    .flipx = 0,
    .flipy = 0,
    .rotation = 0,
    .z_layer = 0,
    .z_order = 0,
    .is_trigger = false,
    .red = 0,
    .green = 0,
    .blue = 0,
    .duration = 0.0f,
    .opacity = 1.0f,
    .color_channel = 1,
    .player_color_1 = false,
    .player_color_2 = false,
    .blending = false,
};

Color parseColor(char* colorString) {
    Color color = {
        .color = 0 | 0xff,
        .blending = false
    };

    SYS_Report("color string: %s\n", colorString);

    for (char* key = strtok(colorString, "_"); key && key[0]; key = strtok(NULL, "_")) {
        char* value = strtok(NULL, "_");
        // should always be key-value pairs
        assert(value && value[0]);

        switch (atoi(key)) {
            case 1:
                color.color |= (u8) atoi(value) << 24;
                break;
            case 2:
                color.color |= (u8) atoi(value) << 16;
                break;
            case 3:
                color.color |= (u8) atoi(value) << 8;
                break;
            // TODO: Who knows what keys 4 and 5 are??? They're def not this...
            // case 4:
            //     color.color |= (u8) atoi(value);
            //     break;
            // case 5:
            //     color.blending = atoi(value) == 1;
            //     break;
        }
    }

    SYS_Report("Resulting color: 0x%08x, blending = %d\n", color.color, color.blending);

    return color;
}

void parseLevelData(char* levelString, char** levelStringPtr, LevelData* levelData) {
    bool last;
    int length;

    do {
        char* key = getNextData(levelString, &length, &levelString, &last);

        assert(key[0] == 'k');
        int val2 = atoi(&key[2]);

        char* value = getNextData(levelString, &length, &levelString, &last);

        switch (key[1]){
            case 'A': {
                int ival = atoi(value);
                f32 fval = atof(value);
                switch (val2){
                    case 2:
                        levelData->gamemode = ival;
                        break;
                    case 3:
                        levelData->minimode = ival;
                        break;
                    case 4:
                        levelData->speed = ival;
                        break;
                    case 5:
                        levelData->obj2Blending = ival;
                        break;
                    case 6:
                        levelData->BGTexId = ival;
                        break;
                    case 7:
                        levelData->groundTexId = ival;
                        break;
                    case 8:
                        levelData->dualMode = ival;
                        break;
                    case 9:
                        levelData->startPosObj = ival;
                        break;
                    case 10:
                        levelData->multiPlayerMode = ival;
                        break;
                    case 11:
                        levelData->flipGravity = ival;
                        break;
                    case 13:
                        levelData->songOffset = fval;
                        break;
                    case 14:
                        // levelData.guideline = value
                        break;
                    case 15:
                        levelData->fadeIn = ival;
                        break;
                    case 16:
                        levelData->fadeOut = ival;
                        break;
                    case 17:
                        levelData->groundLineId = ival;
                        break;
                    case 18:
                        levelData->font = ival;
                        break;
                    case 20:
                        levelData->reverseMode = ival;
                        break;
                    case 22:
                        levelData->platformerMode = ival;
                        break;
                    case 25:
                        levelData->middlegroundTexId = ival;
                        break;
                    case 28:
                        levelData->mirrorMode = ival;
                        break;
                    case 29:
                        levelData->rotateGameplay = ival;
                        break;
                    default:
                        SYS_Report("Warning: Unhandled level data key %s\n", key);
                        break;
                }
                break;
            }

            case 'S': {
                switch (val2) {
                    case 29:
                        colorConfigPush(&levelData->colorConfig, 1000, parseColor(value));
                        break;
                    case 30:
                        colorConfigPush(&levelData->colorConfig, 1001, parseColor(value));
                        break;
                    case 31:
                        colorConfigPush(&levelData->colorConfig, 1002, parseColor(value));
                        break;
                    case 32:
                        colorConfigPush(&levelData->colorConfig, 1004, parseColor(value));
                        break;
                    case 33:
                        colorConfigPush(&levelData->colorConfig, 1, parseColor(value));
                        break;
                    case 34:
                        colorConfigPush(&levelData->colorConfig, 2, parseColor(value));
                        break;
                    case 35:
                        colorConfigPush(&levelData->colorConfig, 3, parseColor(value));
                        break;
                    case 36:
                        colorConfigPush(&levelData->colorConfig, 4, parseColor(value));
                        break;
                    case 37:
                        colorConfigPush(&levelData->colorConfig, 1003, parseColor(value));
                        break;
                    default:
                        SYS_Report("Warning: Unhandled level data key %s\n", key);
                        break;
                }
                break;
            }

            default:
                SYS_Report("Warning: Unhandled level data key %s\n", key);
                break;
        }
    } while (!last);

    *levelStringPtr = levelString;
}

LevelObject* GDL_ParseLevelString(char* levelString, int* size, ObjectData* dataObjects, LevelData* levelData) {
    int objectsCapacity = 100;
    LevelObject* objects = malloc(objectsCapacity * sizeof(LevelObject));
    if (!objects) {
        return NULL;
    }
    int objectsSize = 0;

    bool expectKey = true;
    int key = 0;

    LevelObject object = defaultObject;
   
    int length;
    bool last;

    levelData->colorConfig.length = 0;

    // Skip level settings (for now)
    parseLevelData(levelString, &levelString, levelData);

    // Parse level objects
    do {
        char* start = getNextData(levelString, &length, &levelString, &last);

        if (levelString == NULL) {
            break;
        }

        if (expectKey) {
            key = atoi(start);
            if (key == 0) {
                SYS_Report("Invalid integer key: %.*s\n", length, start);
            }
        } else {
            int ival = atoi(start);
            float fval = atof(start);

            switch (key) {
                case 1:
                    object.id = ival;
                    object.z_layer = dataObjects[object.id].default_z_layer;
                    object.z_order = dataObjects[object.id].default_z_order;
                    object.base_color_channel = dataObjects[object.id].default_base_color_channel;
                    object.detail_color_channel = dataObjects[object.id].default_detail_color_channel;
                    break;
                case 2:
                    object.x = fval;
                    break;
                case 3:
                    object.y = fval;
                    break;
                case 4:
                    object.flipx = ival;
                    break;
                case 5:
                    object.flipy = ival;
                    break;
                case 6:
                    object.rotation = fval;
                    break;
                case 7:
                    object.red = ival;
                    break;
                case 8:
                    object.green = ival;
                    break;
                case 9:
                    object.blue = ival;
                    break;
                case 10:
                    object.duration = fval;
                    break;
                case 15:
                    object.player_color_1 = ival;
                    break;
                case 16:
                    object.player_color_2 = ival;
                    break;
                case 17:
                    object.blending = ival;
                    break;
                case 21:
                    object.base_color_channel = ival;
                    break;
                case 22:
                    object.detail_color_channel = ival;
                    break;
                case 23:
                    object.color_channel = ival;
                    break;
                case 24:
                    object.z_layer = ival;
                    break;
                case 25:
                    object.z_order = ival;
                    break;
                case 35:
                    object.opacity = fval;
                    break;
                case 36:
                    object.is_trigger = ival;
                    break;
                default:
                    SYS_Report("Unknown key %d\n", key);
                    break;
            }
        }

        if (last) {
            // const char* texturename = object.id < 100 ? objectTextures[object.id] : NULL;
            // SYS_Report("Object: ID: %4d, x: %7.1f, y: %7.1f, texture name: %s.png\n", object.id, object.x, object.y, texturename);

            if (objectsSize == objectsCapacity) {
                objectsCapacity *= 2;
                LevelObject* newObjects = realloc(objects, objectsCapacity * sizeof(LevelObject));
                if (!newObjects) {
                    free(objects);
                    return NULL;
                }
                objects = newObjects;
            }

            objects[objectsSize] = object;
            objectsSize++;

            object = defaultObject;
        }

        expectKey ^= 1;
    } while (levelString != NULL);

    *size = objectsSize;
    return objects;
}