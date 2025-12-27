#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOMINMAX

#include "raylib.h"
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
typedef struct {
  void *(*create_context)(void);
  void (*destroy_context)(void *ctx);
  void (*init)(void *ctx);
  void (*shutdown)(void *ctx);
  void (*update)(float dt, void *ctx);
  void (*render)(void *ctx);
} AppAPI;
int main() {
  AppAPI api = {0};
  void *app_ctx = NULL;

  InitWindow(800, 600, "Hot-reload Example");
  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();

    // api.update(dt, app_ctx);

    BeginDrawing();
    ClearBackground(RAYWHITE);
    // api.render(app_ctx);
    EndDrawing();

    if (IsKeyPressed(KEY_F5)) {
      printf("Hot-reloading DLL...\n");
    }
  }
  CloseWindow();

  return 0;
}
