#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include "entity.h"
#include <stdio.h>
#include <stdbool.h>

#define SCREEN_W 1920
#define SCREEN_H 1080

// Function to check player collision with background mask
int checkPlayerCollision(SDL_Rect intendedPos, SDL_Surface *worldMask, int dx, int dy)
{
    // Create a temporary Entity-like structure for collision checking
    Entity tempEntity = {
        .posScreen = intendedPos,
        .state = DIR_RIGHT // Default, adjusted in CollisionParfaite_PNG
    };

    // Call existing CollisionParfaite_PNG with movement direction
    return CollisionParfaite_PNG(&tempEntity, worldMask, dx, dy);
}

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
    {
        printf("IMG_Init failed: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Surface *screen = SDL_SetVideoMode(SCREEN_W, SCREEN_H, 32, SDL_SWSURFACE);
    if (!screen)
    {
        printf("SDL_SetVideoMode failed: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_WM_SetCaption("Enemy AI with Collision & Attack", NULL);

    SDL_Surface *world = IMG_Load("War4.png");
    if (!world)
    {
        printf("Failed to load background: %s\n", IMG_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_Surface *worldMask = IMG_Load("War4_mask.png");
    if (!worldMask)
    {
        printf("Failed to load world mask: %s\n", IMG_GetError());
        SDL_FreeSurface(world);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    Player player;
    initPlayer(&player, "player.png", 500, 400);

    Entity enemy;
    initEntity(&enemy, "Swordsman_spritelist.png", 324, 555);

    if (world->w != SCREEN_W || world->h != SCREEN_H)
    {
        SDL_Surface *scaled = SDL_CreateRGBSurface(SDL_SWSURFACE, SCREEN_W, SCREEN_H, 32,
                                                   0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
        if (!scaled)
        {
            printf("Failed to create scaled surface: %s\n", SDL_GetError());
            SDL_FreeSurface(world);
            SDL_FreeSurface(worldMask);
            freePlayer(&player);
            freeEntity(&enemy);
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
        SDL_SoftStretch(world, NULL, scaled, NULL);
        SDL_FreeSurface(world);
        world = scaled;
    }

    bool running = true;
    SDL_Event event;
    SDL_Rect bgPos = {0, 0, SCREEN_W, SCREEN_H};
    initCoinSystem();

    addCoinType("coin.png", 'B', 1);
    addCoinType("coin.png", 'S', 5);
    addCoinType("coin.png", 'G', 10);

    placeCoin('B', 100, 200);
    placeCoin('S', 400, 300);
    placeCoin('G', 700, 150);

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    if (checkPlayerAttackRange(&enemy, player.position))
                    {
                        triggerHurt(&enemy);
                        printf("Player attacked enemy, health: %d\n", enemy.health);
                    }
                }
            }
            else if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_h:
                    triggerHurt(&enemy);
                    break;
                case SDLK_d:
                    triggerDeath(&enemy);
                    break;
                case SDLK_ESCAPE:
                    running = false;
                    break;
                case SDLK_LEFT:
                {
                    SDL_Rect intendedPos = player.position;
                    intendedPos.x -= 50;
                    if (intendedPos.x >= 0 && !checkPlayerCollision(intendedPos, worldMask, -10, 0))
                    {
                        player.position.x = intendedPos.x;
                    }
                    break;
                }
                case SDLK_RIGHT:
                {
                    SDL_Rect intendedPos = player.position;
                    intendedPos.x += 50;
                    if (intendedPos.x <= SCREEN_W - player.position.w && !checkPlayerCollision(intendedPos, worldMask, 10, 0))
                    {
                        player.position.x = intendedPos.x;
                    }
                    break;
                }
                case SDLK_UP:
                {
                    SDL_Rect intendedPos = player.position;
                    intendedPos.y -= 50;
                    if (intendedPos.y >= 0 && !checkPlayerCollision(intendedPos, worldMask, 0, -10))
                    {
                        player.position.y = intendedPos.y;
                    }
                    break;
                }
                case SDLK_DOWN:
                {
                    SDL_Rect intendedPos = player.position;
                    intendedPos.y += 50;
                    if (intendedPos.y <= SCREEN_H - player.position.h && !checkPlayerCollision(intendedPos, worldMask, 0, 10))
                    {
                        player.position.y = intendedPos.y;
                    }
                    break;
                }
                default:
                    break;
                }
            }
        }

        SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 40, 40, 40));
        SDL_BlitSurface(world, NULL, screen, &bgPos);
        updateEntity(&enemy, SCREEN_W, SCREEN_H, player.position, worldMask);
        drawEntity(&enemy, screen);
        drawPlayer(&player, screen);

        int collectedValue = checkCoinCollisions(player.position);
        if (collectedValue > 0)
        {
            printf("Collected %d points! Total: %d\n", collectedValue, totalCoinsCollected);
        }
        drawAllCoins(screen);

        SDL_Flip(screen);
        SDL_Delay(16);
    }

    SDL_FreeSurface(world);
    SDL_FreeSurface(worldMask);
    freePlayer(&player);
    freeEntity(&enemy);
    saveCoinsToFile();
    freeCoinSystem();
    IMG_Quit();
    SDL_Quit();
    return 0;
}