#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "spatial.h"
#include <stdio.h>

int main(void) {

  // Context init
  SpatialContext ctx = {0};
  ctx.state = APP_STATE_IDLE;
  Camera2D camera = {0};
  camera.zoom = 1.0f;
  ctx.camera = camera;

  ctx.window.height = 600;
  ctx.window.width = 800;

  ctx.animation.trig_target = 0.01f;
  ctx.animation.trig_time_current = 0.0f;
  ctx.animation.time_elapsed = 0.0f;
  ctx.animation.current_step = 0;
  ctx.animation.animation_playing = false;
  ctx.animation.state = ANIMATION_IDLE;

  ctx.particles = Particles_create(20000);
  for (size_t i = 0; i < ctx.particles->capacity; i++) {
    ctx.particles->items[i].pos.x = GetRandomValue(0, ctx.window.width);
    ctx.particles->items[i].pos.y = GetRandomValue(0, ctx.window.height);
    ctx.particles->items[i].v.x = (float)(i % 5) * 10.0f - 20.0f;
    ctx.particles->items[i].v.y = (float)(i % 3) * 15.0f - 15.0f;
    ctx.particles->count++;
  }

  const float UPDATE_INTERVAL = 1.0f / 60.0f; // 60 Hz
  float accumulator = 0.0f;
  double updateStart, updateTime = 0;
  // Initialize the window
  InitWindow(ctx.window.width, ctx.window.height, "Raylib Hello World");
  // SetTargetFPS(60);

  while (!WindowShouldClose()) {
    ctx.frameTime = GetFrameTime();
    accumulator += ctx.frameTime;
    handle_input(&ctx);

    while (accumulator >= UPDATE_INTERVAL) {
      TIME_IT("Update", handle_update(&ctx));
      accumulator -= UPDATE_INTERVAL;
    }

    // RENDERING
    BeginDrawing();
    BeginMode2D(camera);

    ClearBackground(RAYWHITE);
    DrawText("Hello, World!", 190, 200, 20, LIGHTGRAY);
    TIME_IT("Render", render(&ctx));
    EndMode2D();
    DrawFPS(10, 10);
    DrawText(TextFormat("Update: %.2f ms", updateTime), 10, 30, 20, GREEN);
    // MOUSE REF
    Vector2 mouseScreen = GetMousePosition();
    Vector2 mouseWorld =
        GetScreenToWorld2D(mouseScreen, camera); // Convert to world space
    DrawCircleV(mouseScreen, 4, DARKGRAY);
    DrawTextEx(GetFontDefault(),
               TextFormat("Screen: [%i, %i] World: [%.2f, %.2f]",
                          (int)mouseScreen.x, (int)mouseScreen.y, mouseWorld.x,
                          mouseWorld.y),
               Vector2Add(mouseScreen, (Vector2){-44, -24}), 20, 2, BLACK);
    EndDrawing();
  }

  // CLEANUP
  CloseWindow();

  return 0;
}
