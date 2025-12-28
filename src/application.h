#pragma once
#include <stdbool.h>
#include <stdint.h>

// application.c
#define Assert(Expression)                                                     \
  if (!(Expression)) {                                                         \
    *(int *)0 = 0;                                                             \
  }
#define KiloBytes(value) ((value) * 1024)
#define MegaBytes(value) ((KiloBytes(value)) * 1024)
#define GigaBytes(value) ((MegaBytes(value)) * 1024)

typedef struct {
  float x, y;
} Position;

typedef struct {
  Position pos;
} GameState;

// GameState *init_gameState() {
//   GameState *result = malloc(sizeof(GameState));
//   if (!result) {
//     assert(0 && "LMAO failed");
//   }
//
//   result->pos.x = 0;
//   result->pos.y = 0;
//
//   return result;
// }

typedef struct {
  bool isInitialized;
  void *permamentMemory;
  size_t permanentMemorySize;
  void *transientMemory;
  size_t transientMemorySize;
} GameMemory;

typedef enum {
  RENDER_RECTANGLE,
  RENDER_CIRCLE,
} RenderCommandType;

typedef struct {
  float r, g, b, a;
} AppColor;  

typedef struct {
  RenderCommandType type;
  union {
    struct {
      float x, y, width, height;
      AppColor color;  // Updated
    } rectangle;
    struct {
      float centerX, centerY, radius;
      AppColor color;  // Updated
    } circle;
  };
} RenderCommand;

#define MAX_RENDER_COMMANDS 1024
typedef struct {
  RenderCommand commands[MAX_RENDER_COMMANDS];
  int count;
} RenderQueue;

#define GAME_UPDATE(name) void name(GameMemory *gameMemory, float frameTime)
typedef GAME_UPDATE(GameUpdate);
GAME_UPDATE(game_update_stub) {
  (void)gameMemory;
  (void)frameTime;
}

void push_render_command(RenderQueue *queue, RenderCommand cmd);