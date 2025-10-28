#include "plist.h"

#include <gccore.h>
#include <stdlib.h>

#define MAX_KEY_LENGTH 100

static int stringSize = 0;

static void parse_vec2(const char* str, vec2* out) {
    sscanf(str, "{%f,%f}", &out->x, &out->y);
}

static void parse_rect(const char* str, vec2* first, vec2* second) {
    sscanf(str, "{{%f,%f},{%f,%f}}", &first->x, &first->y, &second->x, &second->y);
}

static void parse_bool(const char* str, bool* rotated) {
    if (strcmp(str, "false") == 0) {
        *rotated = false;
    } else {
        *rotated = true;
    }
}

void parseFrames(struct xml_node* frames, ht_hash_table* ht, int texIdx) {
    int num_frames = xml_node_children(frames);

    for (int i = 0; i < num_frames; i += 2) {
        struct xml_node* key = xml_node_child(frames, i);
        struct xml_node* frame = xml_node_child(frames, i + 1);

        char keyname[MAX_KEY_LENGTH];
        {
            int keylen = xml_string_length(xml_node_content(key));
            stringSize += keylen + 1;
            xml_string_copy(xml_node_content(key), keyname, MAX_KEY_LENGTH - 1);
            keyname[keylen < MAX_KEY_LENGTH ? keylen : MAX_KEY_LENGTH - 1] = '\0';
        }

        texture_info tex_info = { 0 };

        tex_info.sheetIdx = texIdx;

        int num_items = xml_node_children(frame);

        for (int j = 0; j < num_items; j += 2) {
            struct xml_node* keyNode = xml_node_child(frame, j);
            struct xml_node* valueNode = xml_node_child(frame, j + 1);

            char key[MAX_KEY_LENGTH];
            {
                struct xml_string* str = xml_node_content(keyNode);
                int len = xml_string_length(str);
                xml_string_copy(str, key, MAX_KEY_LENGTH - 1);
                key[len < MAX_KEY_LENGTH ? len : MAX_KEY_LENGTH - 1] = '\0';
            }

            char value[MAX_KEY_LENGTH];
            {
                struct xml_string* str = xml_node_content(valueNode);
                int len = xml_string_length(str);
                xml_string_copy(str, value, MAX_KEY_LENGTH - 1);
                value[len < MAX_KEY_LENGTH ? len : MAX_KEY_LENGTH - 1] = '\0';
            }

            // SYS_Report("{%s: %s}\n", key, value);

            if (strcmp(key, "spriteOffset") == 0) {
                parse_vec2(value, &tex_info.spriteOffset);
            } else if (strcmp(key, "spriteSize") == 0) {
                parse_vec2(value, &tex_info.spriteSize);
            } else if (strcmp(key, "spriteSourceSize") == 0) {
                parse_vec2(value, &tex_info.spriteSourceSize);
            } else if (strcmp(key, "textureRect") == 0) {
                parse_rect(value, &tex_info.textureRect.pos, &tex_info.textureRect.size);
            } else if (strcmp(key, "textureRotated") == 0) {
                char boolValue[6];
                boolValue[4] = '\0';
                boolValue[5] = '\0';
                xml_string_copy(xml_node_name(valueNode), boolValue, 5);
                parse_bool(boolValue, &tex_info.textureRotated);
            }
        }

        // SYS_Report("Rect: (%f, %f) (%fx%f)\n", tex_info.textureRect.pos.x, tex_info.textureRect.pos.y, tex_info.textureRect.size.x, tex_info.textureRect.size.y);

        ht_insert(keyname, tex_info);
    }
}

int getStringSize() {
    return stringSize;
}