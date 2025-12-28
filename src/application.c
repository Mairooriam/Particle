// application.c
#include "application.h"
#include "stdio.h"
// if moving to c++ to prevent name mangling
// extern "C" GAME_UPDATE(game_update) { pos->y++; }
GAME_UPDATE(game_update) {
  Assert(sizeof(GameState) <= gameMemory->permanentMemorySize);

  GameState *gameState = (GameState *)gameMemory->permamentMemory;  
  if (!gameMemory->isInitialized) {
    // Initialize GameState in permanent memory
    gameState->pos.x = 0.0f;
    gameState->pos.y = 0.0f;
    gameMemory->isInitialized = true;
  }
  gameState->pos.x += 1.0f;  
printf("x:%f,y:%f\n", gameState->pos.x, gameState->pos.y);
}
