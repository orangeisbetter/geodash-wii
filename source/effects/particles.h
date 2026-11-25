#pragma once

#include <gccore.h>

// Describes one kind of effect. Fields follow the cocos2d particle format so real effect files can map onto it later
typedef struct {
    f32 lifetime;       // seconds
    f32 lifetimeVar;
    f32 speed;          // units per second
    f32 speedVar;
    f32 angle;          // degrees, 0 = right, 90 = up
    f32 angleVar;
    f32 posVarX;        // spawn spread around the emit point, units
    f32 posVarY;
    f32 gravityX;       // units per second^2
    f32 gravityY;
    f32 startSize;      // units
    f32 startSizeVar;
    f32 endSize;
    u32 startColor;     // RGBA
    u32 endColor;
    f32 rate;           // particles per second, for PTC_EmitContinuous
    int colorChannel;   // if set, particles use this channel's color; startColor/endColor then only give the alpha
    bool additive;
} ParticleEmitter;

void PTC_Reset();

void PTC_Emit(const ParticleEmitter* emitter, f32 x, f32 y, bool flipY, int count);
void PTC_EmitTinted(const ParticleEmitter* emitter, f32 x, f32 y, bool flipY, int count, u32 tint);
void PTC_EmitContinuous(const ParticleEmitter* emitter, f32* timer, f32 x, f32 y, bool flipY, f32 dt);

void PTC_Update(f32 dt);
void PTC_Draw(const Mtx view, f32 mirrorCenter, f32 mirrorScale);