#ifndef ENTITY_H
#define ENTITY_H

#include <SDL/SDL.h>
#include <stdbool.h>

typedef enum {
    DIR_RIGHT = 0,      // Walking right animation row
    DIR_LEFT = 1,       // Walking left animation row
    STATE_HURT = 2,     // Hurt animation row
    STATE_DEAD = 3,     // Death animation row
    STATE_ATTACK_RIGHT = 4,  // Attack right animation row
    STATE_ATTACK_LEFT = 5,   // Attack left animation row
    STATE_CHASE = 6     // Chasing player state
} EntityState;
#define ATTACK_ROW 4

typedef struct {
    SDL_Surface *sprite;
    SDL_Rect posScreen;
    SDL_Rect frameRect;
    
    EntityState state;
    EntityState dirFacing;
    int currentFrame;
    int frameCount;
    int animSpeed;
    Uint32 lastUpdate;
    
    int speed;
    bool isHurt;
    bool isAttacking;
    Uint32 lastAttackTime;
    int attackRange;
    int attackCooldown;
    int health; // Current health
    int maxHealth; // Maximum health

    // Patrol and chase parameters
    int patrolOriginX;    // Starting X position of patrol
    int patrolRange;      // Distance to patrol left and right from origin
    int detectionRange;   // Range to detect player and start chasing
    int followRange;      // Range to stop chasing player
    bool isReturning;     // Flag to indicate returning to patrol
} Entity;

// Entity functions
void initEntity(Entity *e, const char *imgPath, int x, int y);
void updateEntity(Entity *e, int screenW, int screenH, SDL_Rect playerPos, SDL_Surface *worldMask);
void drawEntity(Entity *e, SDL_Surface *screen);
void triggerHurt(Entity *e);
void triggerDeath(Entity *e);
void triggerAttack(Entity *e);
void freeEntity(Entity *e);
int checkPlayerInRange(Entity *enemy, SDL_Rect playerPos);
int CollisionParfaite_PNG(Entity *e, SDL_Surface *worldMask, int dx, int dy);

///////////////////////////////////// PLAYER  //////////////////////////////////////
typedef struct {
    SDL_Surface *sprite;
    SDL_Rect position;
} Player;

// Player functions
void initPlayer(Player *p, const char *imgPath, int x, int y);
void drawPlayer(Player *p, SDL_Surface *screen);
void freePlayer(Player *p);

///////////////////////////////////// SECONDARY ENTITY //////////////////////////////

typedef struct {
    SDL_Surface *sprite;
    SDL_Rect position;
    bool collected;
    int value; // Different coin values (1, 5, 10 etc)
    char type; // Coin type identifier
} Coin;
extern int totalCoinsCollected;
// Coin system functions
void initCoinSystem(void);
void addCoinType(const char* imgPath, char type, int value);
void placeCoin(char type, int x, int y);
void drawAllCoins(SDL_Surface* screen);
int checkCoinCollisions(SDL_Rect player);
void saveCoinsToFile(void);
void loadCoinsFromFile(void);
void freeCoinSystem(void);

#endif