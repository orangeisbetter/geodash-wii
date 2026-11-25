#include "particles.h"

#include <stdlib.h>
#include <math.h>

#include "../util/pool.h"
#include "../color.h"
#include "../gfx.h"

#define MAX_PARTICLES 256

typedef struct {
    const ParticleEmitter* emitter;
    f32 x;
    f32 y;
    f32 velX;
    f32 velY;
    f32 dirY;           // -1 when emitted upside down
    f32 life;
    f32 maxLife;
    f32 startSize;
    u32 startColor;
    u32 endColor;
} Particle;

static struct {
    Particle particles[MAX_PARTICLES];
    PoolSlot poolSlots[MAX_PARTICLES];
    PoolSlot* activeList;
    Pool pool;
    int count;
} particlePool;

// Random value in [-1, 1]
static f32 randomSpread() {
    return (rand() / (f32)RAND_MAX) * 2.0f - 1.0f;
}

void PTC_Reset() {
    Pool_Init(&particlePool.pool, particlePool.poolSlots, MAX_PARTICLES);
    particlePool.activeList = NULL;
    particlePool.count = 0;
}

// Keep the emitter's alpha, take the color from the tint
static u32 tintColor(u32 color, u32 tint) {
    return (tint & 0xffffff00) | (color & 0xff);
}

static void emit(const ParticleEmitter* emitter, f32 x, f32 y, bool flipY, int count, u32 startColor, u32 endColor) {
    for (int i = 0; i < count; i++) {
        int slotIndex = Pool_Acquire(&particlePool.pool, &particlePool.activeList);

        // Out of particles, just skip the rest. If this becomes a problem, increase MAX_PARTICLES
        if (slotIndex == -1) {
            return;
        }
        particlePool.count++;

        Particle* particle = particlePool.particles + slotIndex;

        f32 angle = (emitter->angle + emitter->angleVar * randomSpread()) * (M_PI / 180.0f);
        f32 speed = emitter->speed + emitter->speedVar * randomSpread();

        particle->emitter = emitter;
        particle->x = x + emitter->posVarX * randomSpread();
        particle->y = y + emitter->posVarY * randomSpread();
        particle->velX = cosf(angle) * speed;
        particle->dirY = flipY ? -1.0f : 1.0f;
        particle->velY = sinf(angle) * speed * particle->dirY;
        particle->life = 0.0f;
        particle->maxLife = emitter->lifetime + emitter->lifetimeVar * randomSpread();
        particle->startSize = emitter->startSize + emitter->startSizeVar * randomSpread();
        particle->startColor = startColor;
        particle->endColor = endColor;
    }
}

void PTC_Emit(const ParticleEmitter* emitter, f32 x, f32 y, bool flipY, int count) {
    if (emitter->colorChannel) {
        PTC_EmitTinted(emitter, x, y, flipY, count, getColorChannel(emitter->colorChannel).color);
        return;
    }
    emit(emitter, x, y, flipY, count, emitter->startColor, emitter->endColor);
}

void PTC_EmitTinted(const ParticleEmitter* emitter, f32 x, f32 y, bool flipY, int count, u32 tint) {
    emit(emitter, x, y, flipY, count, tintColor(emitter->startColor, tint), tintColor(emitter->endColor, tint));
}

void PTC_EmitContinuous(const ParticleEmitter* emitter, f32* timer, f32 x, f32 y, bool flipY, f32 dt) {
    f32 interval = 1.0f / emitter->rate;

    *timer += dt;
    while (*timer >= interval) {
        *timer -= interval;
        PTC_Emit(emitter, x, y, flipY, 1);
    }
}

void PTC_Update(f32 dt) {
    PoolSlot* current = particlePool.activeList;
    while (current) {
        unsigned int index = current - particlePool.poolSlots;
        Particle* particle = particlePool.particles + index;

        particle->life += dt;

        if (particle->life >= particle->maxLife) {
            // Particle finished; release it
            PoolSlot* next = current->next;
            Pool_Release(&particlePool.pool, index, &particlePool.activeList);
            particlePool.count--;
            current = next;
            continue;
        }

        particle->velX += particle->emitter->gravityX * dt;
        particle->velY += particle->emitter->gravityY * particle->dirY * dt;
        particle->x += particle->velX * dt;
        particle->y += particle->velY * dt;

        current = current->next;
    }
}

// Draws either the normal or the additive particles in one batch
static void drawParticles(bool additive, f32 mirrorCenter, f32 mirrorScale) {
    int count = 0;
    for (PoolSlot* current = particlePool.activeList; current; current = current->next) {
        Particle* particle = particlePool.particles + (current - particlePool.poolSlots);
        if (particle->emitter->additive == additive) {
            count++;
        }
    }
    if (count == 0) {
        return;
    }

    if (additive) {
        GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_NOOP);
    } else {
        GFX_SetBlendMode(BLEND_ALPHA);
    }

    GX_Begin(GX_QUADS, GX_VTXFMT0, count * 4);

    for (PoolSlot* current = particlePool.activeList; current; current = current->next) {
        unsigned int index = current - particlePool.poolSlots;
        Particle* particle = particlePool.particles + index;
        const ParticleEmitter* emitter = particle->emitter;

        if (emitter->additive != additive) {
            continue;
        }

        f32 t = particle->life / particle->maxLife;
        f32 half = (particle->startSize + (emitter->endSize - particle->startSize) * t) / 2.0f;
        u32 color = lerpColor(particle->startColor, particle->endColor, t);

        // Same horizontal mirror as level objects
        f32 x = mirrorCenter + (particle->x - mirrorCenter) * mirrorScale;
        f32 y = particle->y;

        GFX_Plot(x - half, y + half, 0.0f, 0.0f, 0.0f, color);
        GFX_Plot(x + half, y + half, 0.0f, 0.0f, 0.0f, color);
        GFX_Plot(x + half, y - half, 0.0f, 0.0f, 0.0f, color);
        GFX_Plot(x - half, y - half, 0.0f, 0.0f, 0.0f, color);
    }

    GX_End();
}

void PTC_Draw(const Mtx view, f32 mirrorCenter, f32 mirrorScale) {
    if (particlePool.count == 0) {
        return;
    }

    GX_LoadPosMtxImm((MtxP)view, GX_PNMTX0);
    // Untextured; texture coordinates are still sent because of the vertex format, but ignored
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

    drawParticles(false, mirrorCenter, mirrorScale);
    drawParticles(true, mirrorCenter, mirrorScale);

    GFX_SetBlendMode(BLEND_ALPHA);
    GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
}