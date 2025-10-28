#pragma once

int Base64decode_len(const char* bufcoded);
int Base64encode_len(int len);
void Base64decode(char* bufplain, const char* bufcoded);
void Base64encode(char* encoded, const char* string, int len);