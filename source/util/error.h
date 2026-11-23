#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <gccore.h>

__attribute__((noreturn))
void error(const char* fmt, ...);

__attribute__((noreturn))
void _gdw_assert_fail(const char* expr, const char* file, unsigned int line, const char* function);

#undef assert
#define assert(expr) \
    ({ \
        if (!(expr)) _gdw_assert_fail(#expr, __FILE__, __LINE__, __func__); \
    })
