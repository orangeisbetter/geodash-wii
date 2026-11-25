#include "Player.h"

#include "gamelevel.h"

// Ball physics, measured from the original game (1x speed)
static const float ballGravityScale = 0.6f;   // Ball gravity relative to the cube
static const float ballSwitchVel = -1.5f;     // Push towards the new floor when switching
static const float ballMaxVel = 20.0f;        // Above anything reachable in an 8 block corridor
static const float miniModeJumpScale = 0.8f;
static const float playerSize = 30.0f;
static const float cubeJumpVel = 11.180032f;

bool intersectsRect(Rect* a, Rect* b);
void playerDestroy(Player* player);

Player* playerInit(Player* player) {
    player->x = 0.0f;
    player->y = 0.0f;
    player->scale = 1.0f;
    player->velX = 5.770002f;
    player->velY = 0.0f;
    player->rotation = 0.0f;
    player->gravity = 0.958199f;
    player->jumpVel = cubeJumpVel;
    player->groundBottom = 0.0f;
    player->groundTop = 300.0f;
    player->gravityModifier = 1.0f;
    player->gravityFlipped = false;
    player->miniMode = false;
    player->restrictedZone = false;
    player->grounded = false;
    player->canJump = false;
    player->rising = false;
    player->holding = false;
    player->queuedHold = false;
    player->dead = false;
    player->gamemode = GAMEMODE_CUBE;
    return player;
}

bool playerIsFalling(Player* player) {
    if (player->gravityFlipped) {
        return player->velY > 0.0f;
    } else {
        return player->velY < 0.0f;
    }
}

void playerHitGround(Player* player, bool reverseGravity) {
    player->velY = 0.0f;

    player->grounded = true;
    player->canJump = true;
}

void playerDestroy(Player* player) {
    player->dead = true;
    SYS_Report("Player is dead.\n");
    levelRestart();
}

void playerChangeGamemode(Player* player, Gamemode gamemode) {
    if (gamemode == player->gamemode) {
        return;
    }

    player->gamemode = gamemode;
    player->velY = player->velY / 2.0f;
    player->grounded = false;
    player->canJump = false;

    switch (gamemode) {
        case GAMEMODE_CUBE:
            break;
        case GAMEMODE_SHIP:
            break;
        case GAMEMODE_BALL:
            break;
    }
}

void playerChangeToMiniMode(Player* player, bool mini){
    if(mini){
        player->miniMode = true;
        player->scale = 0.5f;
        player->jumpVel = cubeJumpVel * miniModeJumpScale;
    }else{
        player->miniMode = false;
        player->scale = 1.0f;
        player->jumpVel = cubeJumpVel;
    }
    player->velY = player->velY / 2.0f;
    player->grounded = false;
    player->canJump = false;
}

void flipGravity(Player* player, bool flipped, bool isPortal) {
    if (player->gravityFlipped == flipped) {
        return;
    }

    player->gravityFlipped = flipped;
    player->gravityModifier *= -1.0f;
    if(isPortal){
        player->velY /= 2.0f;
    }
}

// Pad and orb strength in ball mode relative to the cube (measured from the original game)
static const float ballPadScale = 0.6f;
static const float ballOrbScale = 0.7f;

static float playerBumpScale(Player* player, bool isOrb) {
    float totalMultiplier = 1.0f;
    switch (player->gamemode) {
        case GAMEMODE_BALL:
            totalMultiplier = isOrb ? ballOrbScale : ballPadScale;
        break;
    }
    if(player->miniMode){
        totalMultiplier *= miniModeJumpScale;
    }
    return totalMultiplier;
}

void playerPadJump(Player* player, float velocity) {
    player->velY = velocity * playerBumpScale(player, false) * player->gravityModifier;
    player->grounded = false;
}

void playerRingJump(Player* player, float velocity, LevelObject* object) {
    if (player->queuedHold) {
        player->queuedHold = false;

        player->velY = velocity * playerBumpScale(player, true) * player->gravityModifier;
        player->grounded = false;
        object->activated = true;
    }
}

float playerGetMidDistance(Player* player){
    return playerSize / 2 * player->scale;
}

void playerGetRect(Player* player, Rect* rect) {
    rect->x = player->x - playerGetMidDistance(player);
    rect->y = player->y - playerGetMidDistance(player);
    rect->width = playerSize * player->scale;
    rect->height = playerSize * player->scale;
}

void playerCollideWithObject(Player* player, LevelObject* object, ObjectData* class) {
    Rect playerRect;
    playerGetRect(player, &playerRect);

    Rect objectRect = object->hitbox;

    //tolerance scaled for miniMode
    float tolerance = 10.0f * player->scale;

    float halfH = playerRect.height / 2.0f;

    float playerBottom = player->y - halfH;
    float playerTop = player->y + halfH;

    float objectBottom = objectRect.y;
    float objectTop = objectRect.y + objectRect.height;

    if (!player->gravityFlipped || player->gamemode == GAMEMODE_SHIP) {
        // SYS_Report("pb:%f, hh: %f, ot: %f\n", playerBottom, halfH, objectTop);
        if (playerBottom >= objectTop - tolerance) {
            if (player->velY < 0) {
                player->y = objectTop + halfH;
                playerHitGround(player, player->gravityFlipped && player->gamemode == GAMEMODE_SHIP);
                return;
            }
        }
    }

    if (player->gravityFlipped || player->gamemode == GAMEMODE_SHIP) {
        if (playerTop <= objectBottom + tolerance) {
            if (player->velY > 0) {
                player->y = objectBottom - halfH;
                playerHitGround(player, player->gravityFlipped && player->gamemode == GAMEMODE_SHIP);
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
        playerDestroy(player);
    }
}

inline float clamp(float value, float lowerBound, float upperBound) {
    if (value < lowerBound) {
        return lowerBound;
    } else if (value > upperBound) {
        return upperBound;
    } else {
        return value;
    }
}

static void playerUpdateJump(Player* player, float dt) {
    switch (player->gamemode) {
        case GAMEMODE_CUBE:
            if (player->holding && player->canJump) {
                player->rising = true;
                player->grounded = false;
                player->canJump = false;
                player->queuedHold = false;

                player->velY = player->jumpVel * player->gravityModifier;
            } else {
                if (!player->rising) {
                    player->canJump = false;

                    player->velY -= player->gravity * player->gravityModifier * dt;

                    if (player->gravityFlipped) {
                        if (player->velY > 15.0f) {
                            player->velY = 15.0f;
                        }
                    } else {
                        if (player->velY < -15.0f) {
                            player->velY = -15.0f;
                        }
                    }

                    if (playerIsFalling(player)) {
                        if (player->gravityFlipped && player->velY > 4.0f) {
                            player->grounded = false;
                        } else if (!player->gravityFlipped && player->velY < -4.0f) {
                            player->grounded = false;
                        }
                    }
                } else {
                    player->velY -= player->gravity * player->gravityModifier * dt;

                    if (playerIsFalling(player)) {
                        player->rising = false;
                        player->grounded = false;
                    }
                }
            }
            break;
        case GAMEMODE_SHIP: {
            float shipAccel = player->holding ? -1.0f : playerIsFalling(player) ? 0.8f
                                                                                : 1.2f;

            float extraBoost = (player->holding && playerIsFalling(player)) ? 0.5f : 0.4f;

            player->velY -= player->gravity * player->gravityModifier * extraBoost * shipAccel * dt;

            if (player->gravityFlipped) {
                player->velY = clamp(player->velY, -8.0f, 0.8f * 8.0f);
            } else {
                player->velY = clamp(player->velY, 0.8 * -8.0f, 8.0f);
            }

            if (player->holding) {
                player->grounded = false;
            }
        } break;
        case GAMEMODE_BALL:
            // Switch gravity only while touching the ground (or ceiling, when flipped)
            if (player->queuedHold && player->canJump) {
                player->queuedHold = false;
                player->grounded = false;

                flipGravity(player, !player->gravityFlipped, false);
                player->velY = ballSwitchVel * player->gravityModifier;
            }
            player->canJump = false;

            player->velY -= player->gravity * ballGravityScale * player->gravityModifier * dt;

            if (player->gravityFlipped) {
                if (player->velY > ballMaxVel) {
                    player->velY = ballMaxVel;
                }
            } else {
                if (player->velY < -ballMaxVel) {
                    player->velY = -ballMaxVel;
                }
            }
        break;
    }
}

void playerUpdate(Player* player, float dt) {
    dt *= 0.9;

    playerUpdateJump(player, dt);

    player->x += player->velX * dt;
    player->y += player->velY * dt;
}
