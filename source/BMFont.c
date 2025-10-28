#include "BMFont.h"

static int parseKV(const char* line, const char* key) {
    char* pos = strstr(line, key);
    if (pos) {
        int value;
        sscanf(pos + strlen(key) + 1, "%d", &value);
        return value;
    }
    return -1;
}

int parseBMFontFile(const char* filepath, BMFont* font) {
    FILE* file = fopen(filepath, "r");
    if (!file) {
        return 0;
    }

    for (int i = 0; i < 128; i++) {
        font->page.chars[i].valid = false;
    }

    char line[512];

    while (fgets(line, 512, file)) {
        if (strstr(line, "info")) {
            font->info.size = parseKV(line, "size");
        } else if (strstr(line, "common")) {
            font->common.lineHeight = parseKV(line, "lineHeight");
            font->common.base = parseKV(line, "base");
            font->common.scaleW = parseKV(line, "scaleW");
            font->common.scaleH = parseKV(line, "scaleH");
            font->common.pages = parseKV(line, "pages");
        } else if (strstr(line, "char ")) {
            int id = parseKV(line, "id");
            if (id < 0 || id > 127) {
                continue;
            }
            BM_Char character = {
                true,
                parseKV(line, "x"),
                parseKV(line, "y"),
                parseKV(line, "width"),
                parseKV(line, "height"),
                parseKV(line, "xoffset"),
                parseKV(line, "yoffset"),
                parseKV(line, "xadvance"),
            };
            font->page.chars[id] = character;
        }
    }

    fclose(file);

    return 1;
}

void Font_RenderGlyph(BMFont* font, f32* posx, f32* posy, char character) {
    BM_Char glyph = font->page.chars[(unsigned char)character];

    f32 xStart = *posx + glyph.xoffset;
    f32 xEnd = xStart + glyph.width;
    f32 yStart = *posy - glyph.yoffset;
    f32 yEnd = yStart - glyph.height;

    f32 uStart = glyph.x;
    f32 uEnd = uStart + glyph.width;
    f32 vStart = glyph.y;
    f32 vEnd = vStart + glyph.height;

    uStart /= font->common.scaleW;
    uEnd /= font->common.scaleW;
    vStart /= font->common.scaleH;
    vEnd /= font->common.scaleH;

    GX_Position3f32(xStart, yStart, 0.0f);
    GX_Color1u32(0xffffffff);
    GX_TexCoord2f32(uStart, vStart);

    GX_Position3f32(xEnd, yStart, 0.0f);
    GX_Color1u32(0xffffffff);
    GX_TexCoord2f32(uEnd, vStart);

    GX_Position3f32(xEnd, yEnd, 0.0f);
    GX_Color1u32(0xffffffff);
    GX_TexCoord2f32(uEnd, vEnd);

    GX_Position3f32(xStart, yEnd, 0.0f);
    GX_Color1u32(0xffffffff);
    GX_TexCoord2f32(uStart, vEnd);

    *posx += glyph.xadvance;
}

void Font_RenderLine(BMFont* font, f32 fontSize, Font_Alignment align, Mtx modelView, f32* posx, f32* posy, char* line, int length, int width) {
    Mtx posMtx;
    guMtxCopy(modelView, posMtx);

    f32 scaleFactor = fontSize / font->info.size;
    guMtxApplyScale(posMtx, posMtx, scaleFactor, scaleFactor, scaleFactor);

    f32 textWidth = (f32)width;

    if (align != ALIGN_LEFT) {
        if (align == ALIGN_CENTER) {
            guMtxApplyTrans(posMtx, posMtx, -textWidth / 2.0f, 0.0f, 0.0f);
        } else if (align == ALIGN_RIGHT) {
            guMtxApplyTrans(posMtx, posMtx, -textWidth, 0.0f, 0.0f);
        }
    }

    GX_LoadPosMtxImm(posMtx, GX_PNMTX0);

    GX_Begin(GX_QUADS, GX_VTXFMT0, length * 4);

    for (int i = 0; i < length; i++) {
        Font_RenderGlyph(font, posx, posy, *(line++));
    }

    GX_End();
}

Font_Word trimWord(BMFont* font, Font_Word word) {
    while (isspace((int)*word.ptr) && word.length > 0) {
        word.ptr++;
        word.length--;
        word.width -= font->page.chars[(unsigned char)*word.ptr].xadvance;
    }
    while (isspace((int)word.ptr[word.length - 1]) && word.length > 0) {
        word.length--;
        word.width -= font->page.chars[(unsigned char)*word.ptr].xadvance;
    }

    return word;
}

f32 Font_RenderText(BMFont* font, GXTexObj* fontTexture, char* text, f32 fontSize, Font_Alignment align, f32 maxWidth, Mtx modelView) {
    int wordsCapacity = 10;
    Font_Word* words = (Font_Word*)malloc(sizeof(Font_Word) * wordsCapacity);
    if (words == NULL)
        return 0.0f;

    int wordsSize = 0;

    int length = strlen(text);

    int spaceWidth = font->page.chars[' '].xadvance;

    Font_Word currentWord = { text, 0, 0 };
    for (int i = 0; i < length; i++) {
        char current = text[i];
        if (current == ' ' || current == '\n') {
            if (wordsSize + 1 >= wordsCapacity) {
                wordsCapacity *= 2;
                words = (Font_Word*)realloc(words, sizeof(Font_Word) * wordsCapacity);
            }

            if (currentWord.length != 0) {
                // Push the word
                words[wordsSize++] = currentWord;
            }

            // Push the space/newline
            words[wordsSize].ptr = &text[i];
            words[wordsSize].length = 1;
            words[wordsSize].width = spaceWidth;
            wordsSize++;

            currentWord.ptr = &text[i + 1];
            currentWord.length = 0;
            currentWord.width = 0;
        } else {
            currentWord.width += font->page.chars[(unsigned char)text[i]].xadvance;
            currentWord.length++;
        }
    }

    if (currentWord.length != 0) {
        if (wordsSize == wordsCapacity) {
            wordsCapacity += 1;
            words = (Font_Word*)realloc(words, sizeof(Font_Word) * wordsCapacity);
        }
        words[wordsSize++] = currentWord;
    }

    GFX_SetBlendMode(BLEND_ALPHA);

    GFX_LoadTexture(fontTexture, GX_TEXMAP0);

    GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);

    f32 posx = 0.0f;
    f32 posy = 0.0f;

    // Break words into lines
    maxWidth = maxWidth * font->info.size / fontSize;
    int max = roundf(maxWidth);

    Font_Word currentLine = { words[0].ptr, 0, 0 };
    for (int i = 0; i < wordsSize; i++) {
        Font_Word current = words[i];

        int newWidth = currentLine.width + current.width;

        if (*current.ptr == '\n') {
            currentLine.length += current.length;
            currentLine.width += current.width;

            // Break the line at the newline
            currentLine = trimWord(font, currentLine);
            Font_RenderLine(font, fontSize, align, modelView, &posx, &posy, currentLine.ptr, currentLine.length, currentLine.width);
            posx = 0.0f;
            posy -= font->info.size * 1.4f;

            currentLine.ptr = words[i].ptr;
            currentLine.length = current.length;
            currentLine.width = current.width;
        } else if (newWidth > max) {
            // Break the line before this word
            currentLine = trimWord(font, currentLine);
            Font_RenderLine(font, fontSize, align, modelView, &posx, &posy, currentLine.ptr, currentLine.length, currentLine.width);
            posx = 0.0f;
            posy -= font->info.size;

            // Set this word as first for next line
            currentLine.ptr = words[i].ptr;
            currentLine.length = current.length;
            currentLine.width = current.width;
        } else {
            currentLine.length += current.length;
            currentLine.width += current.width;
        }
    }

    if (currentLine.length != 0) {
        currentLine = trimWord(font, currentLine);
        Font_RenderLine(font, fontSize, align, modelView, &posx, &posy, currentLine.ptr, currentLine.length, currentLine.width);
        posy -= font->info.size;
    }

    free(words);

    GFX_SetBlendMode(BLEND_NONE);

    return posy * fontSize / font->info.size;
}