#include "gamelevel.h"

#include <gccore.h>
#include <ogc/lwp_watchdog.h>
#include <stdio.h>
#include <stdlib.h>
#include <mp3player.h>
#include <wiiuse/wpad.h>
#include <math.h>

#include "gamemode.h"
#include "Player.h"
#include "../util/sort.h"
#include "../constants.h"
#include "../parse/level.h"
#include "../render.h"
#include "../color.h"
#include "../gfx.h"
#include "menu.h"

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
    TRIGGER
} CollisionType;

typedef struct {
    float x;
    float y;
    float width;
    float height;
} Rect;

ObjectData* objectDefaults = NULL;

typedef struct {
    float x;
    float y;
    float velX;
    float velY;
    float rotation;
    float gravity;
    float jumpVel;
    float groundTop;
    float groundBottom;
    float gravityModifier;
    bool gravityFlipped;
    bool onGround;
    bool canJump;
    bool isRising;
    bool isHolding;
    bool queuedHold;  // For orbs
    bool isDead;
    Gamemode gameMode;
    LevelObject* touchedRing;
    SpriteInfo sprite;
} Player;

static void Player_init(Player* player) {
    player->x = 0;
    player->y = 15.0f;
    player->velX = 5.770002f;
    player->velY = 0.0f;
    player->rotation = 0.0f;
    player->gravity = 0.958199f;
    player->jumpVel = 11.180032f;
    player->groundBottom = 0.0f;
    player->groundTop = 300.0f;
    player->gravityModifier = 1.0f;
    player->gravityFlipped = false;
    player->onGround = false;
    player->canJump = false;
    player->isRising = false;
    player->isHolding = false;
    player->queuedHold = false;
    player->isDead = false;
    player->gameMode = GAMEMODE_CUBE;

    player->sprite.x = 0.0f;
    player->sprite.y = 0.0f;
    player->sprite.rotation = 0.0f;
    player->sprite.flipx = false;
    player->sprite.flipy = false;
}

static s32 my_reader(void* cb_data, void* dst, s32 len) {
    FILE* f = (FILE*)cb_data;
    return fread(dst, 1, len, f);
}

static int currentLevelId;

static f32 camX;
static f32 camY;

static f32 lastX;

static const f32 defaultSpeed = 10.386f;                           // Units: blocks per second
static const f32 gravity = -0.876f * defaultSpeed * defaultSpeed;  // Units: blocks per second squared

static const f32 velCubeJump = 1.94f;
static const f32 velYellowPad = 2.77f;
// static const f32 velPinkPad = 1.79f;
// static const f32 velRedPad = 3.65f;
static const f32 velBluePad = -1.37f;
static const f32 velYellowOrb = 1.91f;
// static const f32 velPinkOrb = 1.37f;
// static const f32 velRedOrb = 2.68f;
static const f32 velBlueOrb = -1.37f;
// static const f32 velGreenOrb = -1.91f;
// static const f32 velBlackOrb = -2.6f;

static const f32 cameraVerticalBounds_2 = 75.0f;

Player player = { 0 };

static LevelObject* objects;
static int objectsSize;

static RenderCache renderCache = { 0 };

static const f32 viewScale = 1.5f;

static u64 lastFrameRender = 0;

const char* levelNames[] = {
    "Stereo Madness",
    "Back On Track",
    "Polargeist",
    "Dry Out",
    "Base After Base",
    "Can't Let Go",
    "Jumper",
    "Time Machine",
    "Cycles",
    "xStep",
    "Clutterfunk",
    "Theory of Everything",
    "Electroman Adventures",
    "Clubstep",
    "Electrodynamix",
    "Hexagon Force",
    "Blast Processing",
    "*****",
    "Geometrical Dominator",
    "*****",
    "Fingerdash",
    "Dash"
};

static const char* levelMusic[] = {
    "StereoMadness.mp3",
    "BackOnTrack.mp3",
    "Polargeist.mp3",
    "DryOut.mp3",
    "BaseAfterBase.mp3",
    "CantLetGo.mp3",
    "Jumper.mp3",
    "TimeMachine.mp3",
    "Cycles.mp3",
    "xStep.mp3",
    "Clutterfunk.mp3",
    "TheoryOfEverything.mp3",
    "Electroman.mp3",
    "Clubstep.mp3",
    "Electrodynamix.mp3",
    "HexagonForce.mp3",
    "BlastProcessing.mp3",
    "*****",
    "GeometricalDominator.mp3",
    "*****",
    "Fingerdash.mp3",
    "Dash.mp3"
};

static int compare(const void* a, const void* b) {
    int first = ((RenderObject*)a)->z_layer - ((RenderObject*)b)->z_layer;
    if (first != 0) {
        return first;
    }
    return ((RenderObject*)a)->z_order - ((RenderObject*)b)->z_order;
}

bool intersects(const LevelObject* o, const Hitbox* hitbox, const SpriteInfo* player) {
    if (hitbox->type == HITBOX_TYPE_NONE) {
        return false;
    }
    // SYS_Report("player x: %f, y: %f\n hitbox x: %f, y: %f, width: %f, height %f\n", b->x, b->y, a->x, a->y, a->width, a->height);
    return !(hitbox->x + o->x + hitbox->width / 2 <= player->x - 15.0 ||
             hitbox->x + o->x - hitbox->width / 2 >= player->x + 15.0 ||
             hitbox->y + o->y + hitbox->height / 2 <= player->y - 15.0 ||
             hitbox->y + o->y - hitbox->height / 2 >= player->y + 15.0);
}

bool intersectsY(const LevelObject* o, const Hitbox* hitbox, const SpriteInfo* player) {
    if (hitbox->type == HITBOX_TYPE_NONE) {
        return false;
    }
    // SYS_Report("player x: %f, y: %f\n hitbox x: %f, y: %f, width: %f, height %f\n", b->x, b->y, a->x, a->y, a->width, a->height);
    return !(
        hitbox->y + o->y + hitbox->height / 2 <= player->y - 15.0 ||
        hitbox->y + o->y - hitbox->height / 2 >= player->y + 15.0);
}

bool intersectsRect(Rect* a, Rect* b) {
    return (a->x <= b->x + b->width &&
            a->x + a->width >= b->x &&
            a->y <= b->y + b->height &&
            a->y + a->height >= b->y);
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
        case 35:  // yellow
        case 67:  // blue
            return PAD;
        case 10:  // blue
        case 11:  // yellow
        case 12:  // cube
        case 13:  // ship
        case 45:
        case 46:
        case 47:
        case 99:
            return PORTAL;
        case 36:  // yellow
        case 84:  // blue
            return ORB;
        default:
            return NONE;  // no collision
    }
}

void hitGround(Player* player, bool reverseGravity) {
    player->velY = 0.0f;

    player->onGround = true;
    player->canJump = true;

    // float rotation = player->rotation;
    // if (rotation != 0.0f) {
    //     int degrees = (int)rotation;
    //     if (degrees < 0) {
    //         degrees %= 360;
    //     } else {
    //         degrees = degrees % 360 + 360;
    //     }

    //     if (!player->gravityFlipped) {
    //         degrees = roundf((float)degrees / 90) * 90;
    //     } else {
    //         degrees = roundf((float)degrees / -90) * -90;
    //     }

    //     player->rotation = (float)degrees;
    // }
}

void flipGravity(Player* player, bool flipped) {
    if (player->gravityFlipped == flipped) {
        return;
    }

    player->gravityFlipped = flipped;
    player->gravityModifier *= -1.0f;

    player->velY /= 2.0f;
}

static void destroyPlayer(Player* player) {
    player->isDead = true;
    SYS_Report("Player is dead.\n");
    levelEnter();
}

static void propelPlayer(Player* player, float strength) {
    player->onGround = false;
    player->velY = player->gravityModifier * strength;
}

static void ringJump(Player* player) {
    if (player->touchedRing != NULL && player->queuedHold && player->isHolding) {
        player->queuedHold = false;
        player->onGround = false;
        player->velY = player->jumpVel * player->gravityModifier;
        player->touchedRing = NULL;
    }
}

void collidedWithObject(Player* player, LevelObject* object) {
    Rect playerRect = {
        player->x - 15.0f,
        player->y - 15.0f,
        30.0f,
        30.0f
    };

    Hitbox* hb = &objectDefaults[object->id].hitbox;

    Rect objectRect = {
        object->x + hb->x - hb->width / 2,
        object->y + hb->y - hb->height / 2,
        hb->width,
        hb->height
    };

    f32 leeway = 10.0f;

    float halfH = playerRect.height / 2.0f;

    float bottomPlayer = player->y - halfH;
    float topPlayer = player->y + halfH;

    float objMaxY = objectRect.y + objectRect.height;
    float objMinY = objectRect.y;

    if (!player->gravityFlipped || player->gameMode == GAMEMODE_SHIP) {
        if (bottomPlayer >= objMaxY - leeway) {
            if (player->velY < 0) {
                player->y = objMaxY + halfH;
                hitGround(player, player->gravityFlipped && player->gameMode == GAMEMODE_SHIP);
                return;
            }
        }
    }

    if (player->gravityFlipped || player->gameMode == GAMEMODE_SHIP) {
        if (topPlayer <= objMinY + leeway) {
            if (player->velY > 0) {
                player->y = objMinY - halfH;
                hitGround(player, !player->gravityFlipped && player->gameMode == GAMEMODE_SHIP);
                return;
            }
        }
    }

    Rect smallRect = {
        objectRect.x + objectRect.width / 2.0f - objectRect.width / 2.0f * 0.3f,
        objectRect.y + objectRect.height / 2.0f - objectRect.height / 2.0f * 0.3f,
        objectRect.width * 0.3f,
        objectRect.height * 0.3f
    };
    if (intersectsRect(&playerRect, &smallRect)) {
        destroyPlayer(player);
    }
}

void checkCollisions(Player* player, float dt) {
    // Check collision with ground
    if (player->y < 15 && player->gameMode != GAMEMODE_SHIP) {
        if (player->gravityFlipped) {
            destroyPlayer(player);
            return;
        }
        player->y = 15;
        hitGround(player, false);
    } else if (player->y > 1200) {
        destroyPlayer(player);
        return;
    }

    if (player->gameMode == GAMEMODE_SHIP) {
        if (player->y > player->groundTop - 15.0f) {
            player->y = player->groundTop - 15.0f;
            hitGround(player, !player->gravityFlipped);
        } else if (player->y < player->groundBottom + 15.0f) {
            player->y = player->groundBottom + 15.0f;
            hitGround(player, player->gravityFlipped);
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

    Rect playerRect = {
        player->x - 15.0f,
        player->y - 15.0f,
        30.0f,
        30.0f
    };

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

        Hitbox* hb = &objectDefaults[object->id].hitbox;

        Rect objectRect = {
            object->x + hb->x - hb->width / 2,
            object->y + hb->y - hb->height / 2,
            hb->width,
            hb->height
        };

        if (intersectsRect(&playerRect, &objectRect)) {
            switch (getCollisionType(object)) {
                case PORTAL:
                    switch (object->id) {
                        case 10:
                            flipGravity(player, false);
                            break;
                        case 11:
                            flipGravity(player, true);
                            break;
                        case 12:
                            player->gameMode = GAMEMODE_CUBE;
                            break;
                        case 13:
                            player->gameMode = GAMEMODE_SHIP;
                            break;
                    }
                    break;
                case PAD:
                    propelPlayer(player, 16.0f);
                    break;
                case ORB:
                    player->touchedRing = object;
                    ringJump(player);
                    break;
                default:
                    collidedWithObject(player, object);
                    break;
            }
        }
    }

    for (int i = 0; i < hazardsCount; i++) {
        LevelObject* hazard = hazards[i];

        Hitbox* hb = &objectDefaults[hazard->id].hitbox;

        Rect hazardRect = {
            hazard->x + hb->x - hb->width / 2,
            hazard->y + hb->y - hb->height / 2,
            hb->width,
            hb->height
        };

        if (intersectsRect(&playerRect, &hazardRect)) {
            destroyPlayer(player);
        }
    }
    // Clear hazards list
}

int loadLevel(int levelId) {
    currentLevelId = levelId;

    u64 start = gettime();

    char levelPath[64];
    snprintf(levelPath, 64, ASSETS_PATH "levels/%d.txt", levelId + 1);

    char* levelString = GDL_GetLevelString(levelPath);
    if (levelString == NULL) {
        return 0;
    }

    objects = GDL_ParseLevelString(levelString, &objectsSize, objectDefaults);
    if (!objects) {
        return 0;
    }

    free(levelString);

    SYS_Report("Level loaded: %d objects. (%f ms)\n", objectsSize, ticks_to_microsecs(gettime() - start) / 1000.0f);

    return 1;
}

static void levelEnter() {
    lastX = 0.0f;

    camX = 0.0f;
    camY = cameraVerticalBounds_2;

    Player_init(&player);

    colorInit();

    if (!RDR_RenderCacheInit(&renderCache, 256)) {
        SYS_Report("Unable to allocate memory for object cache\n");
        exit(EXIT_FAILURE);
    }

    SYS_Report("Render cache created with size %d and capacity %d, p = %p\n", renderCache.size, renderCache.capacity, renderCache.objects);

    MP3Player_Stop();

    {
        char buf[64];
        snprintf(buf, 64, ASSETS_PATH "music/%s", levelMusic[currentLevelId]);
        FILE* file = fopen(buf, "rb");
        MP3Player_PlayFile(file, my_reader, NULL);
    }
}

static void levelExit() {
    MP3Player_Stop();

    free(objects);
    objectsSize = 0;

    RDR_RenderCacheClear(&renderCache);
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

    switch (player->gameMode) {
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

static void updateVisibility() {
}

static bool isFalling(Player* player) {
    // if (player->gravityFlipped) {
    //     return player->velY > 0.0f;
    // } else {
    //     return player->velY < 0.0f;
    // }
    if (player->gravityFlipped) {
        return player->velY > player->gravity + player->gravity;
    } else {
        return player->velY < player->gravity + player->gravity;
    }
}

// Update Y velocity if necessary
static void updateJump(Player* player, f32 dt) {
    switch (player->gameMode) {
        case GAMEMODE_CUBE:
            if (player->isHolding && player->canJump) {
                player->isRising = true;
                player->onGround = false;
                player->canJump = false;
                player->queuedHold = false;

                player->velY = player->jumpVel * player->gravityModifier;
            } else {
                if (!player->isRising) {
                    player->canJump = false;

                    player->velY -= player->gravity * dt * player->gravityModifier;

                    // terminal velocity
                    if (player->gravityFlipped) {
                        if (player->velY > 15.0f) {
                            player->velY = 15.0f;
                        }
                    } else {
                        if (player->velY < -15.0f) {
                            player->velY = -15.0f;
                        }
                    }

                    if (isFalling(player)) {
                        if (player->gravityFlipped && player->velY > 4.0f) {
                            player->onGround = false;
                        } else if (!player->gravityFlipped && player->velY < -4.0f) {
                            player->onGround = false;
                        }
                    }
                } else {
                    player->velY -= player->gravity * dt * player->gravityModifier;

                    if (isFalling(player)) {
                        player->isRising = false;
                        player->onGround = false;
                    }
                }
            }
            break;
        case GAMEMODE_SHIP: {
            float shipAccel;
            if (!player->isHolding && !isFalling(player)) {
                shipAccel = 1.2f;
            } else if (player->isHolding) {
                shipAccel = -1.0f;
            } else {
                shipAccel = 0.8f;
            }

            float extraBoost = (player->isHolding && isFalling(player)) ? 0.5f : 0.4f;

            player->velY -= dt * player->gravity * player->gravityModifier * extraBoost * shipAccel;

            if (player->gravityFlipped) {
                if (player->velY < -8.0f) {
                    player->velY = -8.0f;
                }
                if (player->velY > 0.8f * 8.0f) {
                    player->velY = 0.8f * 8.0f;
                }
            } else {
                if (player->velY < 0.8f * -8.0f) {
                    player->velY = 0.8f * -8.0f;
                }
                if (player->velY > 8.0f) {
                    player->velY = 8.0f;
                }
            }
            if (player->isHolding) {
                player->onGround = false;
            }
        } break;
        default:
            break;
    }
}

static void updatePlayer(Player* player, f32 dt) {
    f32 slow_dt = dt * 0.9f;
    updateJump(player, slow_dt);

    // Integrate position
    player->x += player->velX * slow_dt;
    player->y += player->velY * slow_dt;
}

static void update(f32 dt) {
    lastX = player.x;

    f32 deltaFrames = dt * 60.0f;
    if (deltaFrames > 2.0f) {
        deltaFrames = 2.0f;
    }

    f32 step = deltaFrames / 4.0f;
    for (int i = 0; i < 4; i++) {
        updatePlayer(&player, step);
        checkCollisions(&player, step);
    }

    updateCamera(&player);
    updateVisibility();
}

static void levelRun(f32 deltaTime) {
    u32 held = WPAD_ButtonsHeld(0);

    u32 pressed = WPAD_ButtonsDown(0);
    if (pressed & WPAD_BUTTON_A) {
        player.isHolding = true;
        player.queuedHold = true;
    }

    u32 released = WPAD_ButtonsUp(0);
    if (released & WPAD_BUTTON_A) {
        player.isHolding = false;
        player.queuedHold = false;
    }

    update(deltaTime);

    // switch (gameMode) {
    //     case GAMEMODE_CUBE:

    //         break;
    //     case GAMEMODE_SHIP:
    //         const f32 shipGravity = -0.9582 * 30.0f;

    //         if (held & WPAD_BUTTON_A) {
    //             velY -= shipGravity * gravityModifier * deltaTime;
    //         } else {
    //             velY += shipGravity * gravityModifier * deltaTime;
    //         }

    //         // terminal velocity
    //         if (velY * gravityModifier < -1.0f * defaultSpeed) {
    //             velY = -1.0f * gravityModifier * defaultSpeed;
    //         } else if (velY * gravityModifier > 1.0f * defaultSpeed) {
    //             velY = 1.0f * gravityModifier * defaultSpeed;
    //         }

    //         break;
    // }

    if (pressed & WPAD_BUTTON_B) {
        changeState(&GAMESTATE_MENU, .5);
    }

    // player.x += velX * deltaFrames;

    // // Check Hazard collisions
    // for (int i = 0; i < objectsSize; i++) {
    //     LevelObject* object = &objects[i];

    //     Hitbox* objectHitbox = &objectDefaults[object->id].hitbox;
    //     if (intersects(object, objectHitbox, &player)) {
    //         if (getCollisionType(object) == HAZARD) {
    //             SYS_Report("ded\n");
    //             // TODO kill player
    //         }
    //     }
    // }

    // // Check X collisions
    // for (int i = 0; i < objectsSize; i++) {
    //     LevelObject* object = &objects[i];

    //     Hitbox* objectHitbox = &objectDefaults[object->id].hitbox;
    //     if (intersects(object, objectHitbox, &player)) {
    //         // SYS_Report("there was a collision (x)!\n");  // crash the game
    //         float overlapX = fmin(object->x + objectHitbox->x + objectHitbox->width / 2, player.x + 15.0) - fmax(player.x - 15, object->x + objectHitbox->x - objectHitbox->width / 2);

    //         if (player.x < object->x + objectHitbox->x) {
    //             SYS_Report("ded\n");
    //             // player.x -= overlapX;  // move left
    //         }
    //     }
    // }

    // // player.y += velY * deltaFrames;

    if (!player.onGround) {
        switch (player.gameMode) {
            case GAMEMODE_CUBE:
                player.rotation += (5.0f / 23.0f) * defaultSpeed * 180.0f * deltaTime * player.gravityModifier;
                break;

            case GAMEMODE_SHIP:
                player.rotation = -atan2(player.velY, player.velX) * (180.0f / M_PI);
                break;
        }
    } else {
        player.rotation = fmodf(player.rotation, 360.0f);
        if (player.rotation < 0.0f) {
            player.rotation += 360.0f;
        }

        float targetRotation = roundf(player.rotation / 90.0f) * 90.0f;

        player.rotation += (targetRotation - player.rotation) / 3.0f;
    }

    // grounded = false;

    // // Check Y collisions
    // for (int i = 0; i < objectsSize; i++) {
    //     LevelObject* object = &objects[i];

    //     Hitbox* objectHitbox = &objectDefaults[object->id].hitbox;
    //     if (intersects(object, objectHitbox, &player)) {
    //         // SYS_Report("there was a collision (y)!\n");  // crash the game

    //         float overlapY = fmin(object->y + objectHitbox->y + objectHitbox->height / 2, player.y + 15.0) - fmax(player.y - 15, object->y + objectHitbox->y - objectHitbox->height / 2);
    //         if (getCollisionType(object) == SOLID) {
    //             if (player.y > object->y + objectHitbox->y && gravityModifier > 0.0f) {
    //                 player.y += overlapY;  // move up
    //                 velY = 0;
    //                 player.rotation = roundf(player.rotation / 90.0f) * 90.0f;
    //                 grounded = true;
    //             } else if (player.y < object->y + objectHitbox->y && gravityModifier < 0.0f) {
    //                 player.y -= overlapY;  // move down
    //                 velY = 0;
    //                 player.rotation = roundf(player.rotation / 90.0f) * -90.0f;
    //                 grounded = true;
    //             } else {
    //                 SYS_Report("ded\n");
    //             }
    //         }
    //     }
    // }

    // if (player.y < 15) {
    //     velY = 0;
    //     player.y = 15;
    //     player.rotation = roundf(player.rotation / 90.0f) * 90.0f;
    //     grounded = true;
    // }

    // // Check Pad collisions
    // for (int i = 0; i < objectsSize; i++) {
    //     LevelObject* object = &objects[i];

    //     Hitbox* objectHitbox = &objectDefaults[object->id].hitbox;

    //     if (intersects(object, objectHitbox, &player)) {
    //         switch (getCollisionType(object)) {
    //             case PAD:
    //                 switch (object->id) {
    //                     case 35:
    //                         velY = velYellowPad * gravityModifier * defaultSpeed;
    //                         break;
    //                     case 67:
    //                         velY = velBluePad * gravityModifier * defaultSpeed;
    //                         gravityModifier *= -1.0f;
    //                         break;
    //                 }
    //                 break;
    //             case PORTAL:
    //                 switch (object->id) {
    //                     case 10:
    //                         gravityModifier = 1.0f;
    //                         break;
    //                     case 11:
    //                         gravityModifier = -1.0f;
    //                         break;
    //                     case 12:
    //                         gameMode = GAMEMODE_CUBE;
    //                         break;
    //                     case 13:
    //                         gameMode = GAMEMODE_SHIP;
    //                         break;
    //                 }
    //                 break;
    //             case ORB:
    //                 // if (WPAD_ButtonsDown(0) & WPAD_BUTTON_A || bufferedInput) {
    //                 //     if (bufferedInput) {
    //                 //         bufferedInput = false;
    //                 //     }
    //                 //     switch (object->id) {
    //                 //         case 36:
    //                 //             velY = velYellowOrb * gravityModifier * defaultSpeed;
    //                 //             break;
    //                 //         case 84:
    //                 //             velY = velBlueOrb * gravityModifier * defaultSpeed;
    //                 //             gravityModifier *= -1.0f;
    //                 //             break;
    //                 //     }
    //                 // }
    //                 break;
    //         }
    //     }
    // }

    camX = player.x + 2.5f * 30.0f;

    player.sprite.x = player.x;
    player.sprite.y = player.y;
    player.sprite.rotation = player.rotation;

    // if (player.y < camY - cameraVerticalBounds_2) {
    //     camY = player.y + cameraVerticalBounds_2;
    // } else if (player.y > camY + cameraVerticalBounds_2) {
    //     camY = player.y - cameraVerticalBounds_2;
    // }

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
                        setColorChannel(1000, colorObj);
                        break;
                    case 30:
                        setColorChannel(1001, colorObj);
                        break;
                    case 104:
                        setColorChannel(1002, colorObj);
                        break;
                    case 105:
                        setColorChannel(1004, colorObj);
                        break;
                    case 744:
                        setColorChannel(1003, colorObj);
                        break;
                    case 221:
                        setColorChannel(1, colorObj);
                        break;
                    case 717:
                        setColorChannel(2, colorObj);
                        break;
                    case 718:
                        setColorChannel(3, colorObj);
                        break;
                    case 743:
                        setColorChannel(4, colorObj);
                        break;
                    default:
                        setColorChannel(object->color_channel, colorObj);
                        break;
                }
            }
        }
    }
}

static void levelRender() {
    f32 viewBoundsL = camX - (view_width / 2.0f) / viewScale;
    f32 viewBoundsR = camX + (view_width / 2.0f) / viewScale;

    f32 fadeBoundsL = viewBoundsL + 90.0f;
    f32 fadeBoundsR = viewBoundsR - 90.0f;

    // Build render cache
    RDR_RenderCacheClear(&renderCache);

    for (int i = 0; i < objectsSize; i++) {
        LevelObject* object = &objects[i];
        // first check to see if it's on screen
        if (object->x < viewBoundsL || object->x > viewBoundsR) {
            continue;
        }

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

        // then do the things that show up in the render cache
        ObjectData* objectData = &objectDefaults[object->id];
        if (!objectData->exists || objectData->texture == NULL) {
            continue;
        }

        texture_info* tex = ht_search(objectData->texture);
        if (tex == NULL) {
            // SYS_Report("Can't find texture %d\n", object->id);
            continue;
        }

        RenderObject* renderObject = RDR_RenderCacheAdd(&renderCache);
        if (!renderObject) {
            SYS_Report(__FILE__ ": Cannot create render object!!!\n");
        }
        renderObject->x = object->x;
        renderObject->y = object->y;
        renderObject->rotation = object->rotation;
        renderObject->flipx = object->flipx;
        renderObject->flipy = object->flipy;
        renderObject->z_layer = object->z_layer;
        renderObject->z_order = object->z_order;
        renderObject->z = 0;
        renderObject->tex = tex;

        int baseColorChannel = object->base_color_channel;
        int detailColorChannel = object->detail_color_channel;

        if (objectData->swap_base_detail) {
            int tmp = baseColorChannel;
            baseColorChannel = detailColorChannel;
            detailColorChannel = tmp;
        }

        if (objectData->color_type == COLOR_TYPE_BASE) {
            renderObject->colorChannel = baseColorChannel;
        } else if (objectData->color_type == COLOR_TYPE_DETAIL) {
            renderObject->colorChannel = detailColorChannel;
        } else {
            renderObject->colorChannel = 1010;
        }

        // portals
        char* portalString;
        if ((portalString = isPortalId(object->id))) {
            texture_info* tex = ht_search(portalString);
            if (tex == NULL) {
                SYS_Report("Can't find portal texture %d\n", object->id);
                continue;
            }

            RenderObject* renderObject = RDR_RenderCacheAdd(&renderCache);
            if (!renderObject) {
                SYS_Report(__FILE__ ": Cannot create render object!!!\n");
            }

            renderObject->x = object->x;
            renderObject->y = object->y - 2;
            renderObject->rotation = object->rotation;
            renderObject->flipx = object->flipx;
            renderObject->flipy = object->flipy;
            renderObject->z_layer = 4;
            renderObject->z_order = object->z_order;
            renderObject->z = 0;
            renderObject->tex = tex;

            if (objectData->color_type == COLOR_TYPE_BASE) {
                renderObject->colorChannel = baseColorChannel;
            } else if (objectData->color_type == COLOR_TYPE_DETAIL) {
                renderObject->colorChannel = detailColorChannel;
            } else {
                renderObject->colorChannel = 1010;
            }
        }

        // children part
        ObjectDataChild* children = objectData->children;

        for (int i = 0; i < objectDefaults[object->id].numChildren; i++) {
            ObjectDataChild* child = &children[i];
            texture_info* tex = ht_search(child->texture);
            if (tex == NULL) {
                // SYS_Report("Can't find texture %d\n", object->id);
                continue;
            }

            RenderObject* renderObject = RDR_RenderCacheAdd(&renderCache);
            if (!renderObject) {
                SYS_Report(__FILE__ ": Cannot create render object!!!\n");
            }

            renderObject->x = object->x + child->x;
            renderObject->y = object->y + child->y;
            renderObject->rotation = object->rotation + child->rot;
            renderObject->flipx = object->flipx != child->flip_x;
            renderObject->flipy = object->flipy != child->flip_y;
            renderObject->z_layer = object->z_layer;
            renderObject->z_order = object->z_order;
            renderObject->z = child->z;
            renderObject->tex = tex;

            int baseColorChannel = object->base_color_channel;
            int detailColorChannel = object->detail_color_channel;

            if (child->color_type == COLOR_TYPE_BASE) {
                renderObject->colorChannel = baseColorChannel;
            } else if (child->color_type == COLOR_TYPE_DETAIL) {
                renderObject->colorChannel = detailColorChannel;
            } else {
                renderObject->colorChannel = 1010;
            }
        }
    }

    // sort object cache (for layering)
    merge_sort(renderCache.objects, renderCache.size, sizeof(RenderObject), compare);
    // qsort(renderCache.objects, renderCache.size, sizeof(RenderObject), compare);

    // --------------------------------------------------------------------
    // Render graphics
    // --------------------------------------------------------------------

    u64 renderStart = gettime();
    GFX_ResetDrawMode();
    Mtx view;
    guMtxIdentity(view);

    // Background and ground are special, they don't use the camera view matrix. IT'S ALL AN ILLUSION
    RDR_drawBackground(&backgroundTexture, camX, camY, 0.49f, getColorChannel(1000).color);
    RDR_drawGround(&groundTexture, camX, camY, viewScale, getColorChannel(1001).color);

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

    // Render all the objects visible on the screen
    int i = 0;
    for (; i < renderCache.size; i++) {
        RenderObject* object = &renderCache.objects[i];

        if (object->z_layer > 4) {
            break;
        }

        if (object->x < fadeBoundsL) {
            f32 dist = fadeBoundsL - viewBoundsL;
            f32 factor = (fadeBoundsL - object->x) / dist;
            object->y -= factor * 90.0f;
        }
        if (object->x > fadeBoundsR) {
            f32 dist = viewBoundsR - fadeBoundsR;
            f32 factor = (object->x - fadeBoundsR) / dist;
            object->y -= factor * 90.0f;
        }

        RDR_drawSpriteFromMap(object->tex, (SpriteInfo){ object->x, object->y, object->rotation, object->flipx, object->flipy }, object->colorChannel, view);
    }
    switch (player.gameMode) {
        case GAMEMODE_CUBE:
            RDR_drawSpriteFromMap(ht_search("player_01_001.png"), player.sprite, 1011, view);
            RDR_drawSpriteFromMap(ht_search("player_01_2_001.png"), player.sprite, 1012, view);
            break;

        case GAMEMODE_SHIP:
            RDR_drawSpriteFromMap(ht_search("ship_01_001.png"), player.sprite, 1011, view);
            RDR_drawSpriteFromMap(ht_search("ship_01_2_001.png"), player.sprite, 1012, view);
            break;
    }

    for (; i < renderCache.size; i++) {
        RenderObject* object = &renderCache.objects[i];

        if (object->x < fadeBoundsL) {
            f32 dist = fadeBoundsL - viewBoundsL;
            f32 factor = (fadeBoundsL - object->x) / dist;
            object->y -= factor * 90.0f;
        }
        if (object->x > fadeBoundsR) {
            f32 dist = viewBoundsR - fadeBoundsR;
            f32 factor = (object->x - fadeBoundsR) / dist;
            object->y -= factor * 90.0f;
        }

        RDR_drawSpriteFromMap(object->tex, (SpriteInfo){ object->x, object->y, object->rotation, object->flipx, object->flipy }, object->colorChannel, view);
    }

    // Draw line (VERY IMPORTANT)
    RDR_drawLine(view, camY, viewScale);

    GFX_ResetDrawMode();

    // --------------------------------------------------------------------
    // Debug info
    // --------------------------------------------------------------------
    Mtx model;
    guMtxTrans(model, -(view_width / 2.0f) + 10.0f, view_height / 2.0f - 10.0f, 0.0f);
    char buf[100];

    snprintf(buf, 100, "Current level: %s (%d)", levelNames[currentLevelId], currentLevelId + 1);
    f32 offset = Font_RenderText(&font, &fontTexture, buf, 20.0f, ALIGN_LEFT, 600.0f, model);

    snprintf(buf, 100, "pos: %.3f, %.3f", player.x, player.y);
    guMtxTransApply(model, model, 0.0f, offset, 0.0f);
    offset = Font_RenderText(&font, &fontTexture, buf, 16.0f, ALIGN_LEFT, 600.0f, model);

    snprintf(buf, 100, "vel: %.3f, %.3f", player.velX, player.velY);
    guMtxTransApply(model, model, 0.0f, offset, 0.0f);
    offset = Font_RenderText(&font, &fontTexture, buf, 16.0f, ALIGN_LEFT, 600.0f, model);

    snprintf(buf, 100, "Objects: %d/%d (%d/%d bytes)", renderCache.size, objectsSize, renderCache.capacity * sizeof(LevelObject), objectsSize * sizeof(LevelObject));
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
