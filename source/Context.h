#pragma once

typedef struct {
    void (*enter)();
    void (*think)();
    void (*render)();
    void (*exit)();
    void (*suspend)();
    void (*resume)();
} CTX;

// Initialize the context manager
void CTX_Init();

// Enter a context to be executed simultaneously
void CTX_Enter(CTX* context);
// Exit a context
void CTX_Exit(CTX* context);
// Enter a context and suspend all active contexts
void CTX_Override(CTX* context);
// Call the think function of all active contexts
void CTX_Think();
// Call the render function of all active contexts
void CTX_Render();