#include "streak.h"

#include <math.h>

#include "../gfx.h"

#define MAX_STREAK_POINTS 32

static const f32 streakLifetime = 0.25f;    // seconds a point stays in the trail
static const f32 streakWidth = 7.0f;        // units, at the newest point
static const u32 streakColor = 0xffffffa0;  // RGBA at the newest point

typedef struct {
    f32 x;
    f32 y;
    f32 age;
} StreakPoint;

// Ring buffer, newest point at head
static struct {
    StreakPoint points[MAX_STREAK_POINTS];
    int head;
    int count;
} streak;

static StreakPoint* getPoint(int i) {
    return &streak.points[(streak.head - i + MAX_STREAK_POINTS) % MAX_STREAK_POINTS];
}

void STK_Reset() {
    streak.head = 0;
    streak.count = 0;
}

void STK_AddPoint(f32 x, f32 y) {
    streak.head = (streak.head + 1) % MAX_STREAK_POINTS;
    if (streak.count < MAX_STREAK_POINTS) {
        streak.count++;
    }

    StreakPoint* point = &streak.points[streak.head];
    point->x = x;
    point->y = y;
    point->age = 0.0f;
}

void STK_Update(f32 dt) {
    for (int i = 0; i < streak.count; i++) {
        getPoint(i)->age += dt;
    }

    // Oldest points are at the end; drop the ones that expired
    while (streak.count > 0 && getPoint(streak.count - 1)->age >= streakLifetime) {
        streak.count--;
    }
}

void STK_Draw(const Mtx view, f32 mirrorCenter, f32 mirrorScale) {
    if (streak.count < 2) {
        return;
    }

    GX_LoadPosMtxImm((MtxP)view, GX_PNMTX0);
    GFX_SetBlendMode(BLEND_ALPHA);
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

    // Two quads per segment: edge -> center -> edge, so the sides fade out softly
    GX_Begin(GX_QUADS, GX_VTXFMT0, (streak.count - 1) * 8);

    f32 prevX = 0.0f, prevY = 0.0f, prevNX = 0.0f, prevNY = 0.0f;
    u32 prevColor = 0, prevEdge = 0;

    for (int i = 0; i < streak.count; i++) {
        StreakPoint* point = getPoint(i);
        StreakPoint* other = getPoint(i == 0 ? 1 : i - 1);

        f32 x = mirrorCenter + (point->x - mirrorCenter) * mirrorScale;
        f32 y = point->y;
        f32 otherX = mirrorCenter + (other->x - mirrorCenter) * mirrorScale;

        // Direction along the trail, then its perpendicular
        f32 dx = i == 0 ? x - otherX : otherX - x;
        f32 dy = i == 0 ? y - other->y : other->y - y;
        f32 length = sqrtf(dx * dx + dy * dy);
        if (length < 0.001f) {
            dx = 1.0f;
            dy = 0.0f;
            length = 1.0f;
        }

        f32 t = 1.0f - point->age / streakLifetime;
        if (t < 0.0f) {
            t = 0.0f;
        }
        f32 half = streakWidth * t / 2.0f;
        f32 nx = -dy / length * half;
        f32 ny = dx / length * half;

        u32 color = (streakColor & 0xffffff00) | (u32)((streakColor & 0xff) * t);
        u32 edge = streakColor & 0xffffff00;

        if (i > 0) {
            // Upper half
            GFX_Plot(prevX + prevNX, prevY + prevNY, 0.0f, 0.0f, 0.0f, prevEdge);
            GFX_Plot(x + nx, y + ny, 0.0f, 0.0f, 0.0f, edge);
            GFX_Plot(x, y, 0.0f, 0.0f, 0.0f, color);
            GFX_Plot(prevX, prevY, 0.0f, 0.0f, 0.0f, prevColor);
            // Lower half
            GFX_Plot(prevX, prevY, 0.0f, 0.0f, 0.0f, prevColor);
            GFX_Plot(x, y, 0.0f, 0.0f, 0.0f, color);
            GFX_Plot(x - nx, y - ny, 0.0f, 0.0f, 0.0f, edge);
            GFX_Plot(prevX - prevNX, prevY - prevNY, 0.0f, 0.0f, 0.0f, prevEdge);
        }

        prevX = x;
        prevY = y;
        prevNX = nx;
        prevNY = ny;
        prevColor = color;
        prevEdge = edge;
    }

    GX_End();

    GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
}