#include "gamelevel.h"

#include <gccore.h>
#include <ogc/lwp_watchdog.h>
#include <stdio.h>
#include <stdlib.h>
#include <mp3player.h>
#include <wiiuse/wpad.h>
#include <math.h>
#include <ogc/lwp.h>

#include "gamemode.h"
#include "Player.h"
#include "../util/sort.h"
#include "../constants.h"
#include "../parse/level.h"
#include "../render.h"
#include "../color.h"
#include "../gfx.h"
#include "menu.h"
#include "../parse/levelinfo.h"
#include "../effects/particles.h"
#include "../effects/streak.h"

static void levelEnter();
static void levelExit();
static void levelRun(f32 deltaTime);
static void levelRender();

const GameState GAMESTATE_LEVEL = {
    .enter = levelEnter,
    .exit = levelExit,
    .run = levelRun,
    .render = levelRender
};

typedef enum {
    OBJECT_TYPE_DECORATION,
    OBJECT_TYPE_SOLID,
    OBJECT_TYPE_HAZARD,
    OBJECT_TYPE_PAD_YELLOW,
    OBJECT_TYPE_PAD_BLUE,
    OBJECT_TYPE_PAD_PINK,
    OBJECT_TYPE_PAD_RED,
    OBJECT_TYPE_PORTAL_GRAVITY_NORMAL,
    OBJECT_TYPE_PORTAL_GRAVITY_INVERSE,
    OBJECT_TYPE_PORTAL_MIRROR_NORMAL,
    OBJECT_TYPE_PORTAL_MIRROR_INVERSE,
    OBJECT_TYPE_PORTAL_SIZE_REGULAR,
    OBJECT_TYPE_PORTAL_SIZE_MINI,
    OBJECT_TYPE_PORTAL_SHIP,
    OBJECT_TYPE_PORTAL_CUBE,
    OBJECT_TYPE_PORTAL_BALL,
    OBJECT_TYPE_PORTAL_UFO,
    OBJECT_TYPE_ORB_YELLOW,
    OBJECT_TYPE_ORB_BLUE,
    OBJECT_TYPE_ORB_PINK,
    OBJECT_TYPE_ORB_RED,
    OBJECT_TYPE_ORB_GREEN,
    OBJECT_TYPE_ORB_BLACK,
    OBJECT_TYPE_ORB_DASH,
} ObjectType;

typedef enum {
    NONE,
    HAZARD,
    SOLID,
    PAD,
    PORTAL,
    ORB,
    TRIGGER,
    BOOST
} CollisionType;

ObjectData* objectDefaults = NULL;
unsigned int objectDefaultsLength = 0;

static s32 my_reader(void* cb_data, void* dst, s32 len) {
    FILE* f = (FILE*)cb_data;
    return fread(dst, 1, len, f);
}

const static LevelInfo* currentLevel = NULL;

static lwp_t loadThread;
static FILE* music;

static f32 camX;
static f32 camY;
static f32 backgroundPosition;

//effects
static const ParticleEmitter dragEffect = {
    .lifetime = 0.35f,
    .lifetimeVar = 0.15f,
    .speed = 40.0f,
    .speedVar = 20.0f,
    .angle = 120.0f,
    .angleVar = 30.0f,
    .posVarX = 2.0f,
    .posVarY = 1.0f,
    .startSize = 4.0f,
    .startSizeVar = 2.0f,
    .endSize = 1.0f,
    .startColor = 0xffffffff,
    .endColor = 0xffffff00,
    .rate = 40.0f,
    .colorChannel = 1011,
    .additive = true
};
static f32 dragEmitTimer;
static const ParticleEmitter landEffect = {
    .lifetime = 0.25f,
    .lifetimeVar = 0.1f,
    .speed = 60.0f,
    .speedVar = 30.0f,
    .angle = 90.0f,
    .angleVar = 70.0f,
    .posVarX = 12.0f,
    .gravityY = -300.0f,
    .startSize = 4.0f,
    .startSizeVar = 1.5f,
    .endSize = 1.0f,
    .startColor = 0xffffffff,
    .endColor = 0xffffff00,
    .colorChannel = 1011,
    .additive = true
};
static const ParticleEmitter shipFireEffect = {
    .lifetime = 0.2f,
    .lifetimeVar = 0.08f,
    .speed = 30.0f,
    .speedVar = 15.0f,
    .angle = 180.0f,
    .angleVar = 25.0f,
    .posVarX = 1.0f,
    .posVarY = 2.0f,
    .startSize = 4.0f,
    .startSizeVar = 1.5f,
    .endSize = 1.5f,
    .startColor = 0xffffc8ff,
    .endColor = 0xff320000,
    .rate = 80.0f
};
static f32 shipFireTimer;
// Squares floating around the whole screen during ship sections
static const ParticleEmitter shipAmbientEffect = {
    .lifetime = 1.5f,
    .lifetimeVar = 0.5f,
    .speed = 35.0f,
    .speedVar = 10.0f,
    .angle = 205.0f,
    .angleVar = 15.0f,
    .posVarX = 284.0f,
    .posVarY = 150.0f,
    .startSize = 5.0f,
    .startSizeVar = 2.0f,
    .endSize = 2.0f,
    .startColor = 0xffe650ff,
    .endColor = 0xff500000,
    .rate = 10.0f
};
static f32 shipAmbientTimer;

static f32 lastX;
static bool screenFlipped = false;
static f32 mirrorProgress = 0.0f;
static const f32 mirrorTransitionTime = 0.5f;
static bool mirrored = false;

static const f32 baseSpeed = 5.770002f;
// Horizontal speed in units per second, indexed by Speed (level setting kA4 / speed portals) (first one 311.58f)
static const f32 playerSpeeds[] = { baseSpeed, 251.16f / 54, 387.42f / 54, 468.0f / 54, 576.0f / 54 };

// Ball spin in degrees per unit travelled (measured from the original game)
static const f32 ballSpinPerUnit = 1.85f;

static const f32 velCubeJump = 1.94f;
static const f32 velYellowPad = 2.77f;
static const f32 velPinkPad = 1.79f;
// static const f32 velRedPad = 3.65f;
static const f32 velBluePad = -1.37f;
static const f32 velYellowOrb = 1.91f;
static const f32 velPinkOrb = 1.37f;
// static const f32 velRedOrb = 2.68f;
static const f32 velBlueOrb = -1.37f;
// static const f32 velGreenOrb = -1.91f;
// static const f32 velBlackOrb = -2.6f;

static const f32 cameraVerticalBounds_2 = 75.0f;

Player player = { 0 };

static LevelData levelData;
static LevelObject* objects;
static int objectsSize;

static RenderCache renderCache = { 0 };

static struct {
    const LevelObject** objects;
    int size;
    int capacity;
} sortBuffer = {
    NULL,
    0,
    0
};

static const f32 viewScale = 1.5f;

static u64 lastFrameRender = 0;

static int compare(const void* a, const void* b) {
    int first = (*(LevelObject**)a)->z_layer - (*(LevelObject**)b)->z_layer;
    if (first != 0) {
        return first;
    }
    return (*(LevelObject**)a)->z_order - (*(LevelObject**)b)->z_order;
}

bool intersectsRect(Rect* a, Rect* b) {
    return (a->x <= b->x + b->width &&
            a->x + a->width >= b->x &&
            a->y <= b->y + b->height &&
            a->y + a->height >= b->y);
}

//calculate the object hitbox when object is rotated / flipped
void getObjectRect(const LevelObject* object, const Hitbox* hb, Rect* rect){
    static guVector front = { 0.0f, 0.0f, -1.0f };

    // Same flip and rotation as RDR_drawLevelObject (without the screen mirror)
    Mtx transform;
    Mtx temp;
    guMtxScale(transform, object->flipx ? -1.0f : 1.0f, object->flipy ? -1.0f : 1.0f, 1.0f);
    guMtxRotAxisDeg(temp, &front, object->rotation);
    guMtxConcat(temp, transform, transform);

    f32 halfW = hb->width / 2.0f;
    f32 halfH = hb->height / 2.0f;

    guVector corners[4] = {
        { hb->x - halfW, hb->y - halfH, 0.0f },
        { hb->x + halfW, hb->y - halfH, 0.0f },
        { hb->x + halfW, hb->y + halfH, 0.0f },
        { hb->x - halfW, hb->y + halfH, 0.0f }
    };

    // Bounding box of the transformed corners (exact for multiples of 90 degrees)
    f32 minX = 0.0f, maxX = 0.0f, minY = 0.0f, maxY = 0.0f;
    for (int i = 0; i < 4; i++) {
        guVector point;
        guVecMultiply(transform, &corners[i], &point);

        if (i == 0 || point.x < minX) minX = point.x;
        if (i == 0 || point.x > maxX) maxX = point.x;
        if (i == 0 || point.y < minY) minY = point.y;
        if (i == 0 || point.y > maxY) maxY = point.y;
    }

    rect->x = object->x + minX;
    rect->y = object->y + minY;
    rect->width = maxX - minX;
    rect->height = maxY - minY;
}

int getCollisionType(const LevelObject* object) {
    switch (object->id) {
        case 8:
        case 9:
        case 39:
        case 61:
        case 88:
        case 89:
        case 98:
        case 103:
        case 144:
        case 145:
        case 205:
        case 392:
        case 459:
            return HAZARD;
        case 1:
        case 2:
        case 3:
        case 4:
        case 6:
        case 7:
        case 40:
        case 62:
        case 63:
        case 64:
        case 65:
        case 66:
        case 68:
        case 69:
        case 70:
        case 71:
        case 72:
        case 74:
        case 75:
        case 76:
        case 77:
        case 78:
        case 81:
        case 82:
        case 83:
        case 90:
        case 91:
        case 92:
        case 93:
        case 94:
        case 95:
        case 96:
            return SOLID;
        case 35: // yellow
        case 67: // blue
        case 140: // pink
        case 1332: // bump_02_001.png
            return PAD;
        case 10: // blue
        case 11: // yellow
        case 12: // cube
        case 13: // ship
        case 45: // mirror activate
        case 46: // mirror deactivate
        case 47: // ball
        case 99: // changes size deactivate
        case 101: // changes size activate
        case 111: // UFO
        case 286: // dual activate
        case 287: // dual deactivate
        case 660: // wave
        case 745: // robot
        case 747: // teleport activate
        case 749: // teleport deactivate
        case 1331: // spider
        //one more for swing, #18 is in plist but not objects.json
            return PORTAL;
        case 36: // yellow
        case 84: // blue
        case 141: // pink
        // case 1022: //gravJumpRing_01_001.png
        // case 1330: //dropRing_01_001.png
        // case 1333: //ring_02_001.png
        // case 1704: //dashRing_01_001.png
        // case 1751: //dashRing_02_001.png
            return ORB;
        case 200:
        case 201:
        case 202:
        case 203:
        case 1334:
            return BOOST;
        default:
            return NONE; // no collision
    }
}

static bool objectIsUpsideDown(const LevelObject* object) {
    // Upside down is either flipped vertically or rotated roughly 180 degrees, but not both
    f32 rotation = fmodf(object->rotation, 360.0f);
    if (rotation < 0.0f) {
        rotation += 360.0f;
    }
    bool rotated = rotation > 90.0f && rotation < 270.0f;

    return object->flipy != rotated;
}

static bool objectIsHorizontal(const LevelObject* object) {
    // Rotated a quarter turn either way (90 or 270 degrees)
    f32 rotation = fmodf(fabsf(object->rotation), 180.0f);
    return rotation > 45.0f && rotation < 135.0f;
}

void flipScreen(bool state) {
    screenFlipped = state;
}

static void setGroundBounds(Player* player, f32 portalY, f32 height) {
    // Corridor centred on the portal, snapped to whole blocks, never below the real ground
    f32 bottom = floorf((portalY - height / 2.0f) / 30.0f) * 30.0f;
    if (bottom < 0.0f) {
        bottom = 0.0f;
    }
    player->groundBottom = bottom;
    player->groundTop = bottom + height;
}

void checkCollisions(Player* player, float dt) {
    // Check collision with ground
    if (player->y < playerGetMidDistance(player) && player->gamemode != GAMEMODE_SHIP && player->gamemode != GAMEMODE_BALL) {
        if (player->gravityFlipped) {
            playerDestroy(player);
            return;
        }
        player->y = playerGetMidDistance(player);
        playerHitGround(player, false);
    } else if (player->y > 1200) {
        playerDestroy(player);
        return;
    }

    if (player->gamemode == GAMEMODE_SHIP) {
        if (player->y > player->groundTop - playerGetMidDistance(player)) {
            player->y = player->groundTop - playerGetMidDistance(player);
            playerHitGround(player, !player->gravityFlipped);
        } else if (player->y < player->groundBottom + playerGetMidDistance(player)) {
            player->y = player->groundBottom + playerGetMidDistance(player);
            playerHitGround(player, player->gravityFlipped);
        }
    }else if (player->gamemode == GAMEMODE_BALL) {
        if (player->y > player->groundTop - playerGetMidDistance(player)) {
            player->y = player->groundTop - playerGetMidDistance(player);
            if (player->gravityFlipped) {
                playerHitGround(player, false);
            } else if (player->velY > 0.0f) {
                player->velY = 0.0f;
            }
        } else if (player->y < player->groundBottom + playerGetMidDistance(player)) {
            player->y = player->groundBottom + playerGetMidDistance(player);
            if (!player->gravityFlipped) {
                playerHitGround(player, false);
            } else if (player->velY < 0.0f) {
                player->velY = 0.0f;
            }
        }
    }

    // Create hazards list
    static int hazardsCapacity = 0;
    static LevelObject** hazards = NULL;
    int hazardsCount = 0;

    if (!hazards) {
        hazardsCapacity = 100;
        hazards = malloc(sizeof(LevelObject*) * hazardsCapacity);
        if (!hazards) {
            SYS_Report("Error.\n");
            exit(EXIT_FAILURE);
        }
    }

    Rect playerRect;
    playerGetRect(player, &playerRect);

    for (int i = 0; i < objectsSize; i++) {
        LevelObject* object = &objects[i];

        if (getCollisionType(object) == HAZARD) {
            if (hazardsCount == hazardsCapacity) {
                // Resize
                hazards = realloc(hazards, sizeof(LevelObject*) * (hazardsCapacity *= 2));
                if (!hazards) {
                    SYS_Report("Error.\n");
                    exit(EXIT_FAILURE);
                }
            }
            hazards[hazardsCount++] = object;
            continue;
        }

        if (objectDefaults[object->id].hitbox.type != HITBOX_TYPE_BOX) {
            continue;
        }

        if (intersectsRect(&playerRect, &object->hitbox)) {
            switch (getCollisionType(object)) {
                case PORTAL:
                    switch (object->id) {
                        case 10:
                            flipGravity(player, false, true);
                            break;
                        case 11:
                            flipGravity(player, true, true);
                            break;
                        case 12:
                            playerChangeGamemode(player, GAMEMODE_CUBE);
                            break;
                        case 13:
                            playerChangeGamemode(player, GAMEMODE_SHIP);
                            setGroundBounds(player, object->y, 300.0f);
                            break;
                        case 45:
                            flipScreen(true);
                            break;
                        case 46:
                            flipScreen(false);
                            break;
                        case 47:
                            playerChangeGamemode(player, GAMEMODE_BALL);
                            setGroundBounds(player, object->y, 240.0f);
                            break;
                        case 99:
                            playerChangeToMiniMode(player, false);
                        break;
                        case 101:
                            playerChangeToMiniMode(player, true);
                        break;
                    }
                    player->canJump = false;
                    break;
                case PAD:
                    if(object->activated){
                        break;
                    }
                    // SYS_Report("padid: %d\n", object->id);
                    switch(object->id){
                        case 35:
                            playerPadJump(player, velYellowPad * baseSpeed);
                        break;
                        case 67:
                            //SYS_Report("pad rotation: %d\n", object->flipy);
                            if(objectIsUpsideDown(object)){
                                flipGravity(player, false, false);
                            }else{
                                flipGravity(player, true, false);
                            }
                            playerPadJump(player, velBluePad * player->velX);
                            //SYS_Report("velocity after: %f\n", player->velY);
                        break;
                        case 140:
                            playerPadJump(player, velPinkPad * baseSpeed);
                        break;
                    }
                    object->activated = true;
                    break;
                case ORB:
                    if(object->activated){
                        break;
                    }
                    // SYS_Report("orbid: %d\n", object->id);
                    switch(object->id){
                        case 36:
                            playerRingJump(player, velYellowOrb * baseSpeed, object);
                        break;
                        case 84:
                        if (player->queuedHold) {
                            flipGravity(player, !player->gravityFlipped, false);
                            playerRingJump(player, velBlueOrb * player->velX, object);
                        }
                        break;
                        case 141:
                            playerRingJump(player, velPinkOrb * baseSpeed, object);                          
                        break;
                    }
                    break;
                case BOOST:
                    switch(object->id){
                        case 200:
                            player->velX = playerSpeeds[1];
                        break;
                        case 201:
                            player->velX = playerSpeeds[0];
                        break;
                        case 202:
                            player->velX = playerSpeeds[2];
                        break;
                        case 203:
                            player->velX = playerSpeeds[3];
                        break;
                        case 1334:
                            player->velX = playerSpeeds[4];
                        break;
                    }
                break;
                default:
                    playerCollideWithObject(player, object, &objectDefaults[object->id]);
                    break;
            }
        }
    }

    for (int i = 0; i < hazardsCount; i++) {
        LevelObject* hazard = hazards[i];

        if (intersectsRect(&playerRect, &hazard->hitbox)) {
            playerDestroy(player);
        }
    }
    // Clear hazards list
}

void* loadThreadHandler(void* data) {
    u64 start = gettime();

    char levelPath[64];
    snprintf(levelPath, 64, ASSETS_PATH "levels/%d.txt", currentLevel->id);

    char* levelString = GDL_GetLevelString(levelPath);
    if (levelString == NULL) {
        currentLevel = NULL;
        return NULL;
    }
    // SYS_Report("%100s\n", levelString)
    objects = GDL_ParseLevelString(levelString, &objectsSize, objectDefaults, &levelData);
    if (!objects) {
        currentLevel = NULL;
        return NULL;
    }

    //setup object hitboxes
    for (int i = 0; i < objectsSize; i++) {
        LevelObject* object = &objects[i];
        getObjectRect(object, &objectDefaults[object->id].hitbox, &object->hitbox);
    }

    free(levelString);

    char buf[64];
    snprintf(buf, 64, ASSETS_PATH "music/%s", currentLevel->music);
    music = fopen(buf, "rb");

    SYS_Report("Level loaded: %d objects. (%f ms)\n", objectsSize, ticks_to_microsecs(gettime() - start) / 1000.0f);

    return (void*)1;
}

int loadLevel(int levelId) {
    currentLevel = levelStoreSearch(levelId);
    if (!currentLevel) {
        return 0;
    }

    if (LWP_CreateThread(&loadThread, loadThreadHandler, NULL, NULL, 0, 0) != 0) {
        SYS_Report("Could not create load level thread...\n");
    }

    return 1;
}

void levelInit(){
    lastX = 0.0f;

    camX = 0.0f;
    camY = cameraVerticalBounds_2;

    screenFlipped = false;
    mirrorProgress = 0.0f;

    playerInit(&player);
    player.velX = playerSpeeds[levelData.speed];
    colorInit(levelData.colorConfig);

    //effects
    PTC_Reset();
    STK_Reset();
    dragEmitTimer = 0.0f;
    shipFireTimer = 0.0f;
    shipAmbientTimer = 0.0f;

    player.gamemode = levelData.gamemode;
    screenFlipped = levelData.mirrorMode;
    mirrored = levelData.mirrorMode;
    mirrorProgress = levelData.mirrorMode ? 1.0f : 0.0f;
    flipGravity(&player, levelData.flipGravity, false);
    playerChangeToMiniMode(&player, levelData.minimode);
    for (int i = 0; i < objectsSize; i++) {
        objects[i].activated = false;
    }
}

void levelRestart() {
    levelInit();

    MP3Player_Stop();
    fseek(music, 0, SEEK_SET);
    MP3Player_PlayFile(music, my_reader, NULL);
}

static void levelEnter() {
    void* loadThreadRet;
    if (LWP_JoinThread(loadThread, &loadThreadRet) != 0 && (size_t)loadThreadRet == 0) {
        SYS_Report("Level loading failed!!!\n");
    }

    levelInit();

    sortBuffer.capacity = 256;
    sortBuffer.objects = malloc(sizeof(*sortBuffer.objects) * sortBuffer.capacity);
    if (!sortBuffer.objects) {
        SYS_Report("Unable to allocate memory for sort buffer!\n");
        exit(EXIT_FAILURE);
    }

    MP3Player_Stop();
    MP3Player_PlayFile(music, my_reader, NULL);
}

static void levelExit() {
    MP3Player_Stop();
    fclose(music);

    free(objects);
    objectsSize = 0;

    free(sortBuffer.objects);
    sortBuffer.objects = NULL;
    sortBuffer.capacity = 0;
    sortBuffer.size = 0;
}

static void updateCamera(Player* player) {
    float halfHeight = view_height / viewScale / 2.0f;

    float screenTop = camY + halfHeight;
    float screenBottom = camY - halfHeight;

    float upperBound = 90.0f;
    float lowerBound = 120.0f;

    if (player->gravityFlipped) {
        // Swap
        float temp = upperBound;
        upperBound = lowerBound;
        lowerBound = temp;
    }

    float candidatePosition = camY;
    float easeValue = 10.0f;

    switch (player->gamemode) {
        case GAMEMODE_CUBE:
        default:
            if (player->y + upperBound > screenTop) {
                candidatePosition = player->y + upperBound - halfHeight;
            } else if (player->y - lowerBound < screenBottom) {
                candidatePosition = player->y - lowerBound + halfHeight;
            }
            break;
        case GAMEMODE_SHIP:
        case GAMEMODE_BALL:
            candidatePosition = (player->groundTop + player->groundBottom) / 2.0f;
            easeValue = 30.0f;
            break;
    }

    if (candidatePosition - halfHeight < -60.0f) {
        candidatePosition = -60.0f + halfHeight;
    }

    camY += (candidatePosition - camY) / easeValue;
}

static void updateMirror(f32 dt){
    f32 step = dt / mirrorTransitionTime;

    if (screenFlipped) {
        mirrorProgress += step;
        if (mirrorProgress > 1.0f) {
            mirrorProgress = 1.0f;
        }
    } else {
        mirrorProgress -= step;
        if (mirrorProgress < 0.0f) {
            mirrorProgress = 0.0f;
        }
    }
}

static void updateVisibility() {
}

static void update(f32 dt) {
    lastX = player.x;
    bool landed = false;

    f32 deltaFrames = dt * 60.0f;
    if (deltaFrames > 2.0f) {
        deltaFrames = 2.0f;
    }

    f32 step = deltaFrames / 4.0f;
    for (int i = 0; i < 4; i++) {
        bool wasGrounded = player.grounded;
        playerUpdate(&player, step);
        checkCollisions(&player, step);
        if (!wasGrounded && player.grounded) {
            landed = true;
        }
    }

    // A press can only activate an orb during the frame it happened
    // player.queuedHold = false; apparently actually this is allowed
    
    backgroundPosition += player.velX * deltaFrames * 0.9f * (-mirrored * 2 + 1);
    updateCamera(&player);
    updateMirror(dt);
    updateVisibility();
    
    //effects
    if (player.gamemode == GAMEMODE_CUBE && landed && !player.dead) {
        f32 halfSize = 15.0f * player.scale;
        PTC_Emit(&landEffect, player.x, player.y - halfSize * player.gravityModifier, player.gravityFlipped, 10);
    }
    if (player.gamemode == GAMEMODE_CUBE && player.grounded && !player.dead) {
        f32 halfSize = 15.0f * player.scale;
        PTC_EmitContinuous(&dragEffect, &dragEmitTimer, player.x - halfSize, player.y - halfSize * player.gravityModifier, player.gravityFlipped, dt);
    }
    if (player.gamemode == GAMEMODE_SHIP && !player.dead) {
        // Back of the ship, rotated with the sprite (positive rotation is clockwise)
        f32 angle = player.rotation * (M_PI / 180.0f);
        f32 localX = -16.0f * player.scale;
        f32 localY = -3.0f * player.scale * player.gravityModifier;
        f32 backX = player.x + localX * cosf(angle) + localY * sinf(angle);
        f32 backY = player.y - localX * sinf(angle) + localY * cosf(angle);

        PTC_EmitContinuous(&shipFireEffect, &shipFireTimer, backX, backY, player.gravityFlipped, dt);
        PTC_EmitContinuous(&shipAmbientEffect, &shipAmbientTimer, camX, (player.groundTop + player.groundBottom) / 2.0f, false, dt);
        STK_AddPoint(backX, backY);
    }
    PTC_Update(dt);
    STK_Update(dt);
}

static void levelRun(f32 deltaTime) {
    u32 pressed = WPAD_ButtonsDown(0);
    if (pressed & WPAD_BUTTON_A) {
        player.holding = true;
        player.queuedHold = true;
    }

    u32 released = WPAD_ButtonsUp(0);
    if (released & WPAD_BUTTON_A) {
        player.holding = false;
        player.queuedHold = false;
    }

    update(deltaTime);

    if (pressed & WPAD_BUTTON_B) {
        changeState(&GAMESTATE_MENU, .5);
    }

    switch (player.gamemode) {
        case GAMEMODE_CUBE:
            if (!player.grounded) {
                player.rotation += (5.0f / 23.0f) * 10.386f * 180.0f * deltaTime * player.gravityModifier;
            } else {
                player.rotation = fmodf(player.rotation, 360.0f);
                if (player.rotation < 0.0f) {
                    player.rotation += 360.0f;
                }

                float targetRotation = roundf(player.rotation / 90.0f) * 90.0f;

                player.rotation += (targetRotation - player.rotation) / 3.0f;
            }
            break;
        case GAMEMODE_SHIP:
            player.rotation = -atan2(player.velY, player.velX) * (180.0f / M_PI);
            break;

        case GAMEMODE_BALL:
            // velX * 0.9 units per physics frame, 60 frames per second
            player.rotation += ballSpinPerUnit * player.velX * 0.9f * 60.0f * deltaTime * player.gravityModifier;
        break;
    }

    camX = player.x + 2.5f * 30.0f;

    f32 viewBoundsL = camX - (view_width / 2.0f) / viewScale;
    f32 viewBoundsR = camX + (view_width / 2.0f) / viewScale;

    for (int i = 0; i < objectsSize; i++) {
        LevelObject* object = &objects[i];

        if (object->x < viewBoundsL || object->x > viewBoundsR) {
            continue;
        }

        // next check to see if it's a color trigger

        if (
            object->id == 29 ||
            object->id == 30 ||
            object->id == 104 ||
            object->id == 105 ||
            object->id == 744 ||
            object->id == 221 ||
            object->id == 717 ||
            object->id == 718 ||
            object->id == 743 ||
            object->id == 899 ||
            object->is_trigger) {
            if (object->x > lastX && object->x <= player.x) {
                u32 color = ((u32)object->red << 24) + ((u32)object->green << 16) + ((u32)object->blue << 8) + ((u32)(object->opacity * 255));
                Color colorObj = { color, object->blending };
                switch (object->id) {
                    case 29:
                        setColorChannel(1000, colorObj, object->duration);
                        break;
                    case 30:
                        setColorChannel(1001, colorObj, object->duration);
                        break;
                    case 104:
                        setColorChannel(1002, colorObj, object->duration);
                        break;
                    case 105:
                        setColorChannel(1004, colorObj, object->duration);
                        break;
                    case 744:
                        setColorChannel(1003, colorObj, object->duration);
                        break;
                    case 221:
                        setColorChannel(1, colorObj, object->duration);
                        break;
                    case 717:
                        setColorChannel(2, colorObj, object->duration);
                        break;
                    case 718:
                        setColorChannel(3, colorObj, object->duration);
                        break;
                    case 743:
                        setColorChannel(4, colorObj, object->duration);
                        break;
                    default:
                        setColorChannel(object->color_channel, colorObj, object->duration);
                        break;
                }
            }
        }
    }
}

void figureOutPortalOffset(LevelObject* portalObject){
    //SYS_Report("portal rotation of id %d: %f\n", portalObject->id, portalObject->rotation);

    f32 defaultOffset = 1.0f;
    switch(portalObject->id){
        case 45:
            defaultOffset = 0;
        break;
        case 13:
        case 47:
            defaultOffset = 2;
        break;
    }
    if(portalObject->rotation == 90.0f){
        portalObject->x -= defaultOffset;
    }else if(portalObject->rotation == 270.0f || portalObject->rotation == -90.0f){
        portalObject->x += defaultOffset;
    }else{
        portalObject->y -= defaultOffset;
    }   
}

// Edge fade and mirror offset for an object, returns its alpha
static u8 getObjectModel(const LevelObject* object, f32 mirrorScale, Mtx model) {
    f32 viewBoundsL = camX - (view_width / 2.0f) / viewScale;
    f32 viewBoundsR = camX + (view_width / 2.0f) / viewScale;
    f32 fadeBoundsL = viewBoundsL + 90.0f;
    f32 fadeBoundsR = viewBoundsR - 90.0f;

    f32 scale = 1.0f;
    u8 alpha = 255;

    if (object->x < fadeBoundsL) {
        f32 factor = (fadeBoundsL - object->x) / (fadeBoundsL - viewBoundsL);
        alpha = 255 * (1.0f - factor);
        scale = 1.0f - factor;
    }
    if (object->x > fadeBoundsR) {
        f32 factor = (object->x - fadeBoundsR) / (viewBoundsR - fadeBoundsR);
        alpha = 255 * (1.0f - factor);
        scale = 1.0f - factor;
    }

    f32 x = (object->x - camX) * (mirrorScale - 1.0f);

    guMtxScale(model, scale, scale, 1.0f);
    guMtxTransApply(model, model, x, 0.0f, 0.0f);
    return alpha;
}

static void levelRender() {
    f32 mirrorScale = cosf(mirrorProgress * M_PI);
    mirrored = mirrorScale < 0.0f;

    f32 viewBoundsL = camX - (view_width / 2.0f) / viewScale;
    f32 viewBoundsR = camX + (view_width / 2.0f) / viewScale;

    f32 fadeBoundsL = viewBoundsL + 90.0f;
    f32 fadeBoundsR = viewBoundsR - 90.0f;

    // Clear buffer
    sortBuffer.size = 0;

    for (int i = 0; i < objectsSize; i++) {
        LevelObject* object = &objects[i];
        // first check to see if it's on screen
        if (object->x < viewBoundsL || object->x > viewBoundsR) {
            continue;
        }
        
            //SYS_Report("this unknown object is on screen: %s\n", objectDefaults[object->id].texture);
        

        // then check to see it it's an animation trigger

        if (object->id == 22 ||
            object->id == 23 ||
            object->id == 24 ||
            object->id == 25 ||
            object->id == 26 ||
            object->id == 27 ||
            object->id == 28 ||
            object->id == 55 ||
            object->id == 56 ||
            object->id == 57 ||
            object->id == 58 ||
            object->id == 59) {
            continue;
        }

        // Expand buffer if necessary
        if (sortBuffer.size == sortBuffer.capacity) {
            sortBuffer.capacity *= 2;
            sortBuffer.objects = realloc(sortBuffer.objects, sizeof(*sortBuffer.objects) * sortBuffer.capacity);
            if (!sortBuffer.objects) {
                SYS_Report("Unable to resize sort buffer!\n");
                return;
            }
        }

        sortBuffer.objects[sortBuffer.size++] = object;
    }

    // sort object cache (for z order)
    merge_sort(sortBuffer.objects, sortBuffer.size, sizeof(*sortBuffer.objects), compare);

    // --------------------------------------------------------------------
    // Render graphics
    // --------------------------------------------------------------------

    u64 renderStart = gettime();
    GFX_ResetDrawMode();
    Mtx view;
    guMtxIdentity(view);

    // Background and ground are special, they don't use the camera view matrix. IT'S ALL AN ILLUSION
    RDR_drawBackground(&backgroundTexture, backgroundPosition, camY, 0.45f, getColorChannel(1000).color);
    bool hasCeiling = player.gamemode == GAMEMODE_SHIP || player.gamemode == GAMEMODE_BALL;
    f32 floorY = hasCeiling ? player.groundBottom : 0.0f;

    RDR_drawGround(&groundTexture, backgroundPosition, camY - floorY, viewScale, getColorChannel(1001).color, false);
    if (hasCeiling) {
        RDR_drawGround(&groundTexture, backgroundPosition, camY - player.groundTop, viewScale, getColorChannel(1001).color, true);
    }
    // Set up camera view matrix
    guMtxTrans(view, -camX, -camY, -10.0f);
    guMtxScaleApply(view, view, viewScale, viewScale, 1.0f);

    // Prepare for object rendering
    GFX_SetBlendMode(BLEND_ALPHA);
    GFX_LoadTexture(&spriteSheet, GX_TEXMAP0);
    GFX_LoadTexture(&gameSheet02, GX_TEXMAP1);
    GFX_LoadTexture(&gameSheet03, GX_TEXMAP2);
    GFX_LoadTexture(&gameSheet04, GX_TEXMAP3);
    GFX_LoadTexture(&gameSheetIcons, GX_TEXMAP4);
    GFX_LoadTexture(&gameSheetGlow, GX_TEXMAP5);

    // Glow goes under every object
    for (int j = 0; j < sortBuffer.size; j++) {
        const LevelObject* object = sortBuffer.objects[j];
        Mtx model;
        u8 alpha = getObjectModel(object, mirrorScale, model);
        RDR_drawLevelObjectGlow(object, &objectDefaults[object->id], mirrored, alpha, model, view);
    }

    int i = 0;
    int itoreturnto = 0;
    for (; i < sortBuffer.size; i++) {
        const LevelObject* object = sortBuffer.objects[i];

        if (object->id >= objectDefaultsLength) {
            // unknown object. Skip
            continue;
        }

        if (object->z_layer > 4) {
            //if the object is drawn infront of the player, then go through this list and find objects linked with objects that should be drawn behind the player. namely portals.
            char* portalTexture = isPortalId(object->id);
            if(portalTexture == NULL){
                continue;
            }
        }else{
            itoreturnto++;
        }

        f32 x = 0.0f;
        f32 y = 0.0f;
        f32 scale = 1.0f;
        u8 alpha = 255;

        if (object->x < fadeBoundsL) {
            f32 dist = fadeBoundsL - viewBoundsL;
            f32 factor = (fadeBoundsL - object->x) / dist;
            alpha = 255 * (1.0f - factor);
            // y = -factor * 90.0f;
            scale = 1.0f - factor;
        }
        if (object->x > fadeBoundsR) {
            f32 dist = viewBoundsR - fadeBoundsR;
            f32 factor = (object->x - fadeBoundsR) / dist;
            alpha = 255 * (1.0f - factor);
            // y = -factor * 90.0f;
            scale = 1.0f - factor;
        }

        x = (object->x - camX) * (mirrorScale - 1.0f);

        Mtx model;
        guMtxScale(model, scale, scale, 1.0f);
        guMtxTransApply(model, model, x, y, 0.0f);
        if(object->z_layer <= 4){
            RDR_drawLevelObject(object, &objectDefaults[object->id], mirrored, alpha, model, view);
        }else{
            //portals
            LevelObject notObject;
            notObject = *object;
            //figureOutPortalOffset(&notObject);         
            
            ObjectData notObjectDefaults;
            notObjectDefaults = objectDefaults[object->id];
            notObjectDefaults.exists = true;
            notObjectDefaults.texture = isPortalId(object->id);
            notObjectDefaults.default_z_layer = 0;
            notObjectDefaults.default_z_order = 0;
            notObjectDefaults.numChildren = 0;
            
            RDR_drawLevelObject(&notObject, &notObjectDefaults, mirrored, alpha, model, view);
        }
    }
    i = itoreturnto;//return back to where i should be

    STK_Draw(view, camX, mirrorScale);
    PTC_Draw(view, camX, mirrorScale);
  
    SpriteInfo playerSprite = {
        .x = camX + (player.x - camX) * mirrorScale,
        .y = player.y,
        .rotation = mirrored ? -player.rotation : player.rotation,
        .scale = player.scale,
        .flipx = mirrored,
        .flipy = player.gamemode == GAMEMODE_SHIP && player.gravityFlipped
    };

    switch (player.gamemode) {
        case GAMEMODE_CUBE:
            RDR_drawSpriteFromMap(ht_search("player_01_001.png"), &playerSprite, 1011, view);
            RDR_drawSpriteFromMap(ht_search("player_01_2_001.png"), &playerSprite, 1012, view);
            break;

        case GAMEMODE_SHIP:
            RDR_drawSpriteFromMap(ht_search("ship_01_001.png"), &playerSprite, 1011, view);
            RDR_drawSpriteFromMap(ht_search("ship_01_2_001.png"), &playerSprite, 1012, view);
            break;
        case GAMEMODE_BALL:
            RDR_drawSpriteFromMap(ht_search("player_ball_00_001.png"), &playerSprite, 1011, view);
            RDR_drawSpriteFromMap(ht_search("player_ball_00_2_001.png"), &playerSprite, 1012, view);
    }

    for (; i < sortBuffer.size; i++) {
        const LevelObject* object = sortBuffer.objects[i];

        if (object->id >= objectDefaultsLength) {
            // unknown object. Skip
            continue;
        }

        f32 x = 0.0f;
        f32 y = 0.0f;
        f32 scale = 1.0f;
        u8 alpha = 255;

        if (object->x < fadeBoundsL) {
            f32 dist = fadeBoundsL - viewBoundsL;
            f32 factor = (fadeBoundsL - object->x) / dist;
            alpha = 255 * (1.0f - factor);
            // y = -factor * 90.0f;
            scale = 1.0f - factor;
        }
        if (object->x > fadeBoundsR) {
            f32 dist = viewBoundsR - fadeBoundsR;
            f32 factor = (object->x - fadeBoundsR) / dist;
            alpha = 255 * (1.0f - factor);
            // y = -factor * 90.0f;
            scale = 1.0f - factor;
        }

        x = (object->x - camX) * (mirrorScale - 1.0f);

        Mtx model;
        guMtxScale(model, scale, scale, 1.0f);
        guMtxTransApply(model, model, x, y, 0.0f);
        RDR_drawLevelObject(object, &objectDefaults[object->id], mirrored, alpha, model, view);
    }

    // Draw line (VERY IMPORTANT)
    RDR_drawLine(view, camY - floorY, viewScale);
    if (hasCeiling) {
        RDR_drawLine(view, camY - player.groundTop, viewScale);
    }
    GFX_ResetDrawMode();

    // --------------------------------------------------------------------
    // Debug info
    // --------------------------------------------------------------------
    Mtx model;
    guMtxTrans(model, -(view_width / 2.0f) + 10.0f, view_height / 2.0f - 10.0f, 0.0f);
    char buf[100];

    snprintf(buf, 100, "Current level: %s (%d)", currentLevel->name, currentLevel->id);
    f32 offset = Font_RenderText(&font, &fontTexture, buf, 20.0f, ALIGN_LEFT, 600.0f, model);

    snprintf(buf, 100, "pos: %.3f, %.3f", player.x, player.y);
    guMtxTransApply(model, model, 0.0f, offset, 0.0f);
    offset = Font_RenderText(&font, &fontTexture, buf, 16.0f, ALIGN_LEFT, 600.0f, model);

    snprintf(buf, 100, "vel: %.3f, %.3f", player.velX, player.velY);
    guMtxTransApply(model, model, 0.0f, offset, 0.0f);
    offset = Font_RenderText(&font, &fontTexture, buf, 16.0f, ALIGN_LEFT, 600.0f, model);

    snprintf(buf, 100, "Objects: %d/%d (%u/%u bytes)", sortBuffer.size, objectsSize, sortBuffer.capacity * sizeof(LevelObject), objectsSize * sizeof(LevelObject));
    guMtxTransApply(model, model, 0.0f, offset, 0.0f);
    offset = Font_RenderText(&font, &fontTexture, buf, 16.0f, ALIGN_LEFT, 600.0f, model);

    u64 us = ticks_to_microsecs(lastFrameRender);
    snprintf(buf, 100, "RDR: %2lld.%03lld ms", us / 1000, us % 1000);
    guMtxTransApply(model, model, 0.0f, offset, 0.0f);
    offset = Font_RenderText(&font, &fontTexture, buf, 16.0f, ALIGN_LEFT, 600.0f, model);

    u64 renderTime = gettime() - renderStart;

    lastFrameRender = renderTime;

    // if (HWButton != -1) {
    //     free(objects);
    //     SYS_ResetSystem(SYS_POWEROFF, 0, 0);
    //     // SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
    // }
}
