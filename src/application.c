// application.c
#include "application.h"
#include "stdio.h"
// if moving to c++ to prevent name mangling
// extern "C" GAME_UPDATE(game_update) { pos->y++; }
GAME_UPDATE(game_update) {
  Assert(sizeof(GameState) <= gameMemory->permanentMemorySize);

  GameState *gameState = (GameState *)gameMemory->permamentMemory;
  if (!gameMemory->isInitialized) {
    gameState->pos.x = 0.0f;
    gameState->pos.y = 0.0f;
    gameMemory->isInitialized = true;
  }
  gameState->pos.y = 200.0f;
  gameState->pos.x += 100.0f;

  // printf("x:%f,y:%f\n", gameState->pos.x, gameState->pos.y);

  RenderQueue *renderQueue = (RenderQueue *)gameMemory->transientMemory;
  renderQueue->count = 0;

  RenderCommand rectCmd = {RENDER_RECTANGLE,
                           .rectangle = {gameState->pos.x,
                                         gameState->pos.y,
                                         50,
                                         50,
                                         {.r = 255, .b = 0, .g = 0, .a = 200}}};
  push_render_command(renderQueue, rectCmd);

  RenderCommand circleCmd = {RENDER_CIRCLE,
                             .circle = {gameState->pos.x,
                                        gameState->pos.y,
                                        25,
                                        {.r = 0, .b = 255, .g = 0, .a = 200}}};
  push_render_command(renderQueue, circleCmd);
}

void push_render_command(RenderQueue *queue, RenderCommand cmd) {
  if (queue->count < MAX_RENDER_COMMANDS) {
    queue->commands[queue->count++] = cmd;
  }
}
