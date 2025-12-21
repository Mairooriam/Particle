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
  ctx.y_bound = 300;
  ctx.x_bound = 300;

  ctx.animation.trig_target = 0.01f;
  ctx.animation.trig_time_current = 0.0f;
  ctx.animation.time_elapsed = 0.0f;
  ctx.animation.current_step = 0;
  ctx.animation.animation_playing = false;
  ctx.animation.state = ANIMATION_IDLE;

  const float UPDATE_INTERVAL = 1.0f / 60.0f; // 60 Hz
  float accumulator = 0.0f;
  double updateStart, updateTime = 0;
  // Initialize the window
  InitWindow(ctx.window.width, ctx.window.height, "Raylib Hello World");
  SetTargetFPS(60);

  // init(&ctx, 50);
  init_collision_moving_to_not_moving(&ctx);
  // init_collision_not_moving(&ctx);
  ctx.paused = true;
  while (!WindowShouldClose()) {
    ctx.frameTime = GetFrameTime();
    accumulator += ctx.frameTime;
    handle_input(&ctx);

    if (!ctx.paused || ctx.step_one_frame) {
      TIME_IT("Update", handle_update(&ctx));
      ctx.step_one_frame = false;
    }

    // RENDERING
    BeginDrawing();
    BeginMode2D(ctx.camera);
    ClearBackground(RAYWHITE);
    DrawText("Hello, World!", 190, 200, 20, LIGHTGRAY);
    TIME_IT("Render", render(&ctx));
    EndMode2D();

    DrawFPS(10, 10);
    EndDrawing();
  }

  // CLEANUP
  CloseWindow();

  return 0;
}
