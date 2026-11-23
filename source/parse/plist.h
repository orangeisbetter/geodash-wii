#pragma once

#include "../vendor/xml.h"
#include "../util/hash_table.h"

void parseFrames(struct xml_node* frames, ht_hash_table* ht, int texIdx);

int getStringSize();