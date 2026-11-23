#include "Player.h"

#include "gamelevel.h"

bool intersectsRect(Rect* a, Rect* b);
void playerDestroy(Player* player);

Player* playerInit(Player* player) {
    player->x = 0.0f;
    player->y = 0.0f;
    player->velX = 5.770002f;
    player->velY = 0.0f;
    player->rotation = 0.0f;
    player->gravity = 0.958199f;
    player->jumpVel = 11.180032f;
    player->groundBottom = 0.0f;
    player->groundTop = 300.0f;
    player->gravityModifier = 1.0f;
    player->gravityFlipped = false;
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

void playerPadJump(Player* player, float velocity) {
    player->velY = velocity * player->gravityModifier;
    player->grounded = false;
}

void playerRingJump(Player* player, float strength) {
    if (player->queuedHold) {
        player->queuedHold = false;

        player->velY = player->jumpVel * strength * player->gravityModifier;
        player->grounded = false;
    }
}

void playerGetRect(Player* player, Rect* rect) {
    rect->x = player->x - 15.0f;
    rect->y = player->y - 15.0f;
    rect->width = 30.0f;
    rect->height = 30.0f;
}

void playerCollideWithObject(Player* player, LevelObject* object, ObjectData* class) {
    Rect playerRect;
    playerGetRect(player, &playerRect);

    Hitbox* hb = &class->hitbox;

    Rect objectRect = {
        object->x + hb->x - hb->width / 2.0f,
        object->y + hb->y - hb->height / 2.0f,
        hb->width,
        hb->height
    };

    float tolerance = 10.0f;

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
            break;
    }
}

void playerUpdate(Player* player, float dt) {
    dt *= 0.9;

    playerUpdateJump(player, dt);

    player->x += player->velX * dt;
    player->y += player->velY * dt;
}
