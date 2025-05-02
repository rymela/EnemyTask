#include "entity.h"
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <errno.h>

void initEntity(Entity *e, const char *imgPath, int x, int y) {
    if (!e) {
        printf("Error: NULL entity pointer\n");
        exit(1);
    }
    memset(e, 0, sizeof(Entity)); // Zero-initialize
    e->sprite = IMG_Load(imgPath);
    if (!e->sprite) {
        printf("Error loading sprite %s: %s\n", imgPath, IMG_GetError());
        exit(1);
    }
    printf("Loaded sprite %s: %dx%d pixels\n", imgPath, e->sprite->w, e->sprite->h);

    e->posScreen.x = x;
    e->posScreen.y = y;
    e->posScreen.w = 128;
    e->posScreen.h = 126;

    e->frameRect.x = 0;
    e->frameRect.y = 0;
    e->frameRect.w = 128;
    e->frameRect.h = 126;

    e->state = DIR_RIGHT;
    e->dirFacing = DIR_RIGHT;
    e->currentFrame = 0;
    e->frameCount = 4;
    e->animSpeed = 100;
    e->lastUpdate = SDL_GetTicks();

    e->speed = 3;
    e->isHurt = false;
    e->isAttacking = false;
    e->lastAttackTime = 0;
    e->attackRange = 100;
    e->attackCooldown = 2000;
    e->health = 100;
    e->maxHealth = 100;

    // Initialize patrol and chase parameters
    e->patrolOriginX = x;
    e->patrolRange = 200;    // Patrol 200 pixels left and right
    e->detectionRange = 200; // Detect player within 200 pixels
    e->followRange = 200;    // Stop chasing if player is > 200 pixels away
    e->isReturning = false;
}

void updateEntity(Entity *e, int screenW, int screenH, SDL_Rect playerPos, SDL_Surface *worldMask) {
    if (!e || !e->sprite) {
        printf("Invalid entity or sprite in updateEntity\n");
        return;
    }

    Uint32 currentTime = SDL_GetTicks();

    // Handle dead state
    if (e->state == STATE_DEAD) {
        if (e->currentFrame < e->frameCount - 1) {
            if (currentTime - e->lastUpdate > e->animSpeed) {
                e->currentFrame++;
                e->frameRect.x = e->currentFrame * 128;
                e->frameRect.y = STATE_DEAD * 128;
                e->lastUpdate = currentTime;
            }
        }
        return;
    }

    // Handle attack states
    if (e->state == STATE_ATTACK_RIGHT || e->state == STATE_ATTACK_LEFT) {
        if (currentTime - e->lastUpdate > e->animSpeed) {
            e->currentFrame++;
            
            if (e->currentFrame >= e->frameCount) {
                e->currentFrame = 0;
                e->isAttacking = false;
                e->state = e->dirFacing;
            }
            
            e->frameRect.x = e->currentFrame * 128;
            e->frameRect.y = e->state * 128;
            
            if (e->state == STATE_ATTACK_RIGHT) {
                e->posScreen.x += e->speed;
            } else if (e->state == STATE_ATTACK_LEFT) {
                e->posScreen.x -= e->speed;
            }
            
            e->lastUpdate = currentTime;
        }
        return;
    }

    // Check for attack trigger
    if (!e->isAttacking && 
        currentTime - e->lastAttackTime > e->attackCooldown &&
        checkPlayerInRange(e, playerPos) &&
        e->health > 0) {
        triggerAttack(e);
        return;
    }

    // Handle hurt state
    if (e->state == STATE_HURT) {
        if (currentTime - e->lastUpdate > e->animSpeed) {
            e->currentFrame++;
            if (e->currentFrame >= e->frameCount) {
                e->currentFrame = 0;
                e->isHurt = false;
                e->state = e->dirFacing;
                e->speed = 3;
            }
            e->frameRect.x = e->currentFrame * 128;
            e->frameRect.y = e->state * 128;
            e->lastUpdate = currentTime;
        }
        return;
    }

    // Calculate distance to player
    int enemyCenterX = e->posScreen.x + e->posScreen.w / 2;
    int playerCenterX = playerPos.x + playerPos.w / 2;
    int dxPlayer = playerCenterX - enemyCenterX;
    int distanceToPlayer = abs(dxPlayer);

    // Check if player is vertically aligned (within 10 pixels horizontally)
    bool isVerticallyAligned = abs(dxPlayer) < 10;

    // Check if player is within detection range to start chasing
    if (e->state != STATE_CHASE && distanceToPlayer <= e->detectionRange && e->health > 0 && !isVerticallyAligned) {
        e->state = STATE_CHASE;
        e->isReturning = false;
    }

    // Handle chase state
    if (e->state == STATE_CHASE) {
        // Stop chasing if player is out of follow range or vertically aligned
        if (distanceToPlayer > e->followRange || isVerticallyAligned) {
            e->state = e->dirFacing;
            e->isReturning = true;
        } else {
            // Move towards player
            if (dxPlayer > 0) {
                e->state = DIR_RIGHT;
                e->dirFacing = DIR_RIGHT;
                e->posScreen.x += e->speed;
            } else if (dxPlayer < 0) {
                e->state = DIR_LEFT;
                e->dirFacing = DIR_LEFT;
                e->posScreen.x -= e->speed;
            }
        }
    } else if (e->isReturning) {
        // Return to patrol origin smoothly
        int dxOrigin = e->patrolOriginX - enemyCenterX;
        if (abs(dxOrigin) > 1) { // Continue moving until very close
            if (dxOrigin > 0) {
                e->state = DIR_RIGHT;
                e->dirFacing = DIR_RIGHT;
                e->posScreen.x += e->speed;
            } else {
                e->state = DIR_LEFT;
                e->dirFacing = DIR_LEFT;
                e->posScreen.x -= e->speed;
            }
            // Check for collisions during return
            if (worldMask && CollisionParfaite_PNG(e, worldMask, dxOrigin > 0 ? e->speed : -e->speed, 0)) {
                printf("Collision during return at x=%d\n", e->posScreen.x);
                if (dxOrigin > 0) {
                    e->posScreen.x -= e->speed + 1;
                    e->state = DIR_LEFT;
                    e->dirFacing = DIR_LEFT;
                } else {
                    e->posScreen.x += e->speed + 1;
                    e->state = DIR_RIGHT;
                    e->dirFacing = DIR_RIGHT;
                }
            }
        } else {
            // Close enough, stop returning and resume patrolling
            e->isReturning = false;
            e->state = DIR_RIGHT;
            e->dirFacing = DIR_RIGHT;
        }
    } else {
        // Normal patrolling
        if (e->state == DIR_RIGHT || e->state == DIR_LEFT) {
            // Check if entity is outside patrol range
            int minPatrolX = e->patrolOriginX - e->patrolRange;
            int maxPatrolX = e->patrolOriginX + e->patrolRange - e->posScreen.w;
            if (e->posScreen.x < minPatrolX) {
                // Move right to return to patrol range
                e->state = DIR_RIGHT;
                e->dirFacing = DIR_RIGHT;
                e->posScreen.x += e->speed;
            } else if (e->posScreen.x > maxPatrolX) {
                // Move left to return to patrol range
                e->state = DIR_LEFT;
                e->dirFacing = DIR_LEFT;
                e->posScreen.x -= e->speed;
            } else {
                // Within range, continue normal patrolling
                int dx = (e->state == DIR_RIGHT) ? e->speed : -e->speed;
                int intendedX = e->posScreen.x + dx;

                // Adjust direction at patrol boundaries
                if (intendedX < minPatrolX) {
                    e->state = DIR_RIGHT;
                    e->dirFacing = DIR_RIGHT;
                    intendedX = e->posScreen.x + e->speed;
                } else if (intendedX > maxPatrolX) {
                    e->state = DIR_LEFT;
                    e->dirFacing = DIR_LEFT;
                    intendedX = e->posScreen.x - e->speed;
                }

                e->posScreen.x = intendedX;
            }

            // Check for collisions with environment
            if (worldMask && CollisionParfaite_PNG(e, worldMask, (e->state == DIR_RIGHT) ? e->speed : -e->speed, 0)) {
                printf("Collision detected with environment at x=%d\n", e->posScreen.x);
                if (e->state == DIR_RIGHT) {
                    e->posScreen.x -= e->speed + 1;
                    e->state = DIR_LEFT;
                    e->dirFacing = DIR_LEFT;
                } else {
                    e->posScreen.x += e->speed + 1;
                    e->state = DIR_RIGHT;
                    e->dirFacing = DIR_RIGHT;
                }
            }
        }
    }

    // Update animation only if moving or in a special state
    if (currentTime - e->lastUpdate > e->animSpeed && !isVerticallyAligned &&
        (e->state == DIR_RIGHT || e->state == DIR_LEFT || e->state == STATE_CHASE)) {
        e->currentFrame = (e->currentFrame + 1) % e->frameCount;
        e->frameRect.x = e->currentFrame * 128;
        e->frameRect.y = (e->state == STATE_CHASE) ? e->dirFacing * 128 : e->state * 128;
        e->lastUpdate = currentTime;
    }

    // Ensure entity stays within screen bounds
    if (e->posScreen.x < 0) {
        e->posScreen.x = 0;
        e->state = DIR_RIGHT;
        e->dirFacing = DIR_RIGHT;
    } else if (e->posScreen.x > screenW - e->posScreen.w) {
        e->posScreen.x = screenW - e->posScreen.w;
        e->state = DIR_LEFT;
        e->dirFacing = DIR_LEFT;
    }
}

void drawEntity(Entity *e, SDL_Surface *screen) {
    if (!e || !e->sprite || !screen) {
        printf("Invalid parameters in drawEntity\n");
        return;
    }
    SDL_BlitSurface(e->sprite, &e->frameRect, screen, &e->posScreen);
}

void triggerHurt(Entity *e) {
    if (!e || e->state >= STATE_HURT || e->health <= 0) return;

    e->state = STATE_HURT;
    e->currentFrame = 0;
    e->isHurt = true;
    e->speed = 1;
    e->lastUpdate = SDL_GetTicks();
    e->health -= 20;
    if (e->health <= 0) {
        triggerDeath(e);
    }
}

void triggerDeath(Entity *e) {
    if (!e) return;
    e->state = STATE_DEAD;
    e->currentFrame = 0;
    e->speed = 0;
    e->health = 0;
    e->lastUpdate = SDL_GetTicks();
}

void freeEntity(Entity *e) {
    if (e && e->sprite) {
        SDL_FreeSurface(e->sprite);
        e->sprite = NULL;
    }
}

Uint32 GetPixel_PNG(SDL_Surface *surface, int x, int y) {
    if (!surface || x < 0 || x >= surface->w || y < 0 || y >= surface->h) {
        printf("Invalid pixel access at (%d, %d), surface dimensions: %dx%d\n", x, y, surface->w, surface->h);
        return 0;
    }

    if (SDL_MUSTLOCK(surface)) {
        if (SDL_LockSurface(surface) < 0) {
            printf("Failed to lock surface: %s\n", SDL_GetError());
            return 0;
        }
    }

    int bpp = surface->format->BytesPerPixel;
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    Uint32 pixel = 0;
    memcpy(&pixel, p, bpp);

    if (SDL_MUSTLOCK(surface)) {
        SDL_UnlockSurface(surface);
    }

    SDL_Color rgb;
    SDL_GetRGB(pixel, surface->format, &rgb.r, &rgb.g, &rgb.b);
    int isCollidable = (rgb.r == 255 && rgb.g == 0 && rgb.b == 0) ? 0 : 1;
    printf("Pixel at (%d, %d): R=%d, G=%d, B=%d, Collidable=%d\n", x, y, rgb.r, rgb.g, rgb.b, isCollidable);
    return isCollidable;
}

int CollisionParfaite_PNG(Entity *e, SDL_Surface *worldMask, int dx, int dy) {
    if (!e || !worldMask) {
        printf("Invalid parameters in CollisionParfaite_PNG\n");
        return 0;
    }

    int x = e->posScreen.x;
    int y = e->posScreen.y;
    int w = e->posScreen.w;
    int h = e->posScreen.h;

    int points[4][2];
    if (dx > 0) { // Moving right
        points[0][0] = x + w - 1; points[0][1] = y;
        points[1][0] = x + w - 1; points[1][1] = y + h / 2;
        points[2][0] = x + w - 1; points[2][1] = y + h - 1;
        points[3][0] = x + w / 2; points[3][1] = y + h / 2;
    } else if (dx < 0) { // Moving left
        points[0][0] = x; points[0][1] = y;
        points[1][0] = x; points[1][1] = y + h / 2;
        points[2][0] = x; points[2][1] = y + h - 1;
        points[3][0] = x + w / 2; points[3][1] = y + h / 2;
    } else if (dy > 0) { // Moving down
        points[0][0] = x; points[0][1] = y + h - 1;
        points[1][0] = x + w / 2; points[1][1] = y + h - 1;
        points[2][0] = x + w - 1; points[2][1] = y + h - 1;
        points[3][0] = x + w / 2; points[3][1] = y + h / 2;
    } else if (dy < 0) { // Moving up
        points[0][0] = x; points[0][1] = y;
        points[1][0] = x + w / 2; points[1][1] = y;
        points[2][0] = x + w - 1; points[2][1] = y;
        points[3][0] = x + w / 2; points[3][1] = y + h / 2;
    } else {
        return 0; // No movement
    }

    int collision = 0;
    for (int i = 0; i < 4; i++) {
        int px = points[i][0];
        int py = points[i][1];

        if (px >= 0 && py >= 0 && px < worldMask->w && py < worldMask->h) {
            if (GetPixel_PNG(worldMask, px, py) == 0) {
                printf("Collision detected at point (%d, %d)\n", px, py);
                collision = 1;
                if (dx > 0) {
                    e->posScreen.x = px - w + 1;
                } else if (dx < 0) {
                    e->posScreen.x = px + 1;
                } else if (dy > 0) {
                    e->posScreen.y = py - h + 1;
                } else if (dy < 0) {
                    e->posScreen.y = py + 1;
                }
                break;
            }
        }
    }

    return collision;
}

void triggerAttack(Entity *e) {
    if (!e || e->state == STATE_DEAD || e->health <= 0) return;
    
    if (e->dirFacing == DIR_RIGHT) {
        e->state = STATE_ATTACK_RIGHT;
    } else {
        e->state = STATE_ATTACK_LEFT;
    }
    
    e->isAttacking = true;
    e->currentFrame = 0;
    e->frameRect.x = 0;
    e->frameRect.y = e->state * 128;
    e->lastUpdate = SDL_GetTicks();
    e->lastAttackTime = SDL_GetTicks();
    printf("Entity attacks %s at position (%d, %d), frameRect.y=%d!\n",
           e->state == STATE_ATTACK_RIGHT ? "right" : "left",
           e->posScreen.x, e->posScreen.y, e->frameRect.y);
}

int checkPlayerInRange(Entity *enemy, SDL_Rect playerPos) {
    if (!enemy) return 0;
    int enemyCenterX = enemy->posScreen.x + enemy->posScreen.w/2;
    int enemyCenterY = enemy->posScreen.y + enemy->posScreen.h/2;
    int playerCenterX = playerPos.x + playerPos.w/2;
    int playerCenterY = playerPos.y + playerPos.h/2;
    
    int dx = enemyCenterX - playerCenterX;
    int dy = enemyCenterY - playerCenterY;
    int distanceSquared = dx*dx + dy*dy;
    
    return (distanceSquared < (enemy->attackRange * enemy->attackRange));
}

int checkPlayerAttackRange(Entity *enemy, SDL_Rect playerPos) {
    if (!enemy) return 0;
    int enemyCenterX = enemy->posScreen.x + enemy->posScreen.w/2;
    int enemyCenterY = enemy->posScreen.y + enemy->posScreen.h/2;
    int playerCenterX = playerPos.x + playerPos.w/2;
    int playerCenterY = playerPos.y + playerPos.h/2;
    
    int dx = enemyCenterX - playerCenterX;
    int dy = enemyCenterY - playerCenterY;
    int distanceSquared = dx*dx + dy*dy;
    
    return (distanceSquared < (enemy->attackRange * enemy->attackRange));
}

////////////////////////////////// PLAYER /////////////////////////

void initPlayer(Player *p, const char *imgPath, int x, int y) {
    if (!p) {
        printf("Error: NULL player pointer\n");
        exit(1);
    }
    memset(p, 0, sizeof(Player));
    p->sprite = IMG_Load(imgPath);
    if (!p->sprite) {
        printf("Failed to load player image %s: %s\n", imgPath, IMG_GetError());
        exit(1);
    }
    
    p->position.x = x;
    p->position.y = y;
    p->position.w = p->sprite->w;
    p->position.h = p->sprite->h;
}

void drawPlayer(Player *p, SDL_Surface *screen) {
    if (!p || !p->sprite || !screen) {
        printf("Invalid parameters in drawPlayer\n");
        return;
    }
    SDL_BlitSurface(p->sprite, NULL, screen, &p->position);
}

void freePlayer(Player *p) {
    if (p && p->sprite) {
        SDL_FreeSurface(p->sprite);
        p->sprite = NULL;
    }
}

//////////////////////////////////// COINS ////////////////////////

typedef struct {
    SDL_Surface* sprite;
    char type;
    int value;
} CoinType;

CoinType* coinTypes = NULL;
int coinTypeCount = 0;
Coin* coins = NULL;
int coinCount = 0;
int totalCoinsCollected = 0;
int* collectedByType = NULL;

void initCoinSystem(void) {
    coinTypes = NULL;
    coins = NULL;
    collectedByType = NULL;
    coinTypeCount = 0;
    coinCount = 0;
    totalCoinsCollected = 0;
    printf("Coin system initialized\n");
}

void addCoinType(const char* imgPath, char type, int value) {
    SDL_Surface* loaded = IMG_Load(imgPath);
    if (!loaded) {
        printf("Failed to load coin image %s: %s\n", imgPath, IMG_GetError());
        return;
    }
    
    CoinType* tempCoinTypes = realloc(coinTypes, (coinTypeCount + 1) * sizeof(CoinType));
    int* tempCollectedByType = realloc(collectedByType, (coinTypeCount + 1) * sizeof(int));
    if (!tempCoinTypes || !tempCollectedByType) {
        printf("Memory reallocation failed!\n");
        SDL_FreeSurface(loaded);
        free(tempCoinTypes);
        free(tempCollectedByType);
        return;
    }
    
    coinTypes = tempCoinTypes;
    collectedByType = tempCollectedByType;
    
    coinTypes[coinTypeCount].sprite = loaded;
    coinTypes[coinTypeCount].type = type;
    coinTypes[coinTypeCount].value = value;
    collectedByType[coinTypeCount] = 0;
    printf("Added coin type %c (%s)\n", type, imgPath);
    coinTypeCount++;
}

void placeCoin(char type, int x, int y) {
    if (coinTypeCount == 0) {
        printf("Error: No coin types defined!\n");
        return;
    }
    for (int i = 0; i < coinTypeCount; i++) {
        if (coinTypes[i].type == type) {
            Coin* tempCoins = realloc(coins, (coinCount + 1) * sizeof(Coin));
            if (!tempCoins) {
                printf("Memory reallocation failed!\n");
                return;
            }
            coins = tempCoins;
            
            coins[coinCount].sprite = coinTypes[i].sprite;
            coins[coinCount].position.x = x;
            coins[coinCount].position.y = y;
            coins[coinCount].position.w = coinTypes[i].sprite->w;
            coins[coinCount].position.h = coinTypes[i].sprite->h;
            coins[coinCount].collected = false;
            coins[coinCount].value = coinTypes[i].value;
            coins[coinCount].type = type;
            
            printf("Placed coin %c at (%d,%d)\n", type, x, y);
            coinCount++;
            return;
        }
    }
    printf("Warning: Coin type '%c' not found!\n", type);
}

void drawAllCoins(SDL_Surface* screen) {
    if (!screen || !coins) return;
    
    for (int i = 0; i < coinCount; i++) {
        if (!coins[i].collected && coins[i].sprite) {
            SDL_BlitSurface(coins[i].sprite, NULL, screen, &coins[i].position);
        }
    }
}

int checkCoinCollisions(SDL_Rect player) {
    if (!coins || !coinTypes) return 0;

    int collectedValue = 0;
    
    for (int i = 0; i < coinCount; i++) {
        if (!coins[i].collected &&
            player.x < coins[i].position.x + coins[i].position.w &&
            player.x + player.w > coins[i].position.x &&
            player.y < coins[i].position.y + coins[i].position.h &&
            player.y + player.h > coins[i].position.y) {
            
            coins[i].collected = true;
            collectedValue += coins[i].value;
            totalCoinsCollected += coins[i].value;
            
            for (int j = 0; j < coinTypeCount; j++) {
                if (coinTypes[j].type == coins[i].type) {
                    collectedByType[j]++;
                    break;
                }
            }
        }
    }
    
    if (collectedValue > 0) {
        saveCoinsToFile();
    }
    
    return collectedValue;
}

void saveCoinsToFile(void) {
    FILE* file = fopen("coins.txt", "w");
    if (!file) {
        printf("Failed to open coins.txt for writing: %s\n", strerror(errno));
        return;
    }
    for (int i = 0; i < coinTypeCount; i++) {
        fprintf(file, "%c %d\n", coinTypes[i].type, collectedByType[i]);
    }
    fclose(file);
}

void loadCoinsFromFile(void) {
    FILE* file = fopen("coins.txt", "r");
    if (!file) {
        printf("Failed to open coins.txt for reading: %s\n", strerror(errno));
        return;
    }
    char type;
    int count;
    while (fscanf(file, "%c %d\n", &type, &count) == 2) {
        for (int i = 0; i < coinTypeCount; i++) {
            if (coinTypes[i].type == type) {
                collectedByType[i] = count;
                break;
            }
        }
    }
    fclose(file);
}

void freeCoinSystem(void) {
    for (int i = 0; i < coinTypeCount; i++) {
        if (coinTypes[i].sprite) {
            SDL_FreeSurface(coinTypes[i].sprite);
        }
    }
    free(coinTypes);
    free(coins);
    free(collectedByType);
    coinTypes = NULL;
    coins = NULL;
    collectedByType = NULL;
    coinTypeCount = 0;
    coinCount = 0;
    totalCoinsCollected = 0;
}