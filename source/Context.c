#include "Context.h"

#include <malloc.h>

static CTX** contextStack;
static int contextSize;
static int contextCapacity;

void ctxAdd(CTX* context) {
    if (contextSize == contextCapacity) {
        contextCapacity *= 2;
        contextStack = realloc(contextStack, sizeof(CTX*) * contextCapacity);
    }

    contextStack[contextSize++] = context;
}

void CTX_Init() {
    contextCapacity = 10;
    contextStack = malloc(sizeof(CTX*) * contextCapacity);
    contextSize = 0;
}

void CTX_Deinit() {
    free(contextStack);
}

// Enter a context to be executed simultaneously
void CTX_Enter(CTX* context) {
}
// Exit a context
void CTX_Exit(CTX* context) {
}
// Enter a context and suspend all active contexts
void CTX_Override(CTX* context) {
}
// Call the think function of all active contexts
void CTX_Think() {
}
// Call the render function of all active contexts
void CTX_Render() {
}