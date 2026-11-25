#include "error.h"

#include <stdarg.h>
#include <stdio.h>
#include <ogc/machine/processor.h>

void error(const char* fmt, ...) {
    va_list list;
    va_start(list, fmt);

    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, list);

    fprintf(stderr, "Error: %s\n", buf);

    va_end(list);

    exit(1);

    // TODO: Display error on screen using console
    while (1) {
        // do nothing... forever...
    }
}

void _gdw_assert_fail(const char* expr, const char* file, unsigned int line, const char* function) {
    error("Assertion failed at %s:%d (%s): %s\n", file, line, function, expr);
}
