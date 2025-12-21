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
  // TODO: crashes if i change bounds
  ctx.y_bound = 600;
  ctx.x_bound = 800;

  ctx.animation.trig_target = 0.01f;
  ctx.animation.trig_time_current = 0.0f;
  ctx.animation.time_elapsed = 0.0f;
  ctx.animation.current_step = 0;
  ctx.animation.animation_playing = false;
  ctx.animation.state = ANIMATION_IDLE;

  ctx.entitySize = 5.0f;

  const float UPDATE_INTERVAL = 1.0f / 60.0f; // 60 Hz
  float accumulator = 0.0f;
  double updateStart, updateTime = 0;
  // Initialize the window
  InitWindow(ctx.window.width, ctx.window.height, "Raylib Hello World");
  SetTargetFPS(60);

  // init(&ctx, 5000);
  // init_collision_moving_to_not_moving(&ctx);
  // init_collision_not_moving(&ctx);
  ctx.paused = true;
  init_collision_diagonal(&ctx);
  // init_collision_single_particle(&ctx);
  ctx.sGrid.bX = ctx.x_bound;
  ctx.sGrid.bY = ctx.y_bound;
  ctx.sGrid.spacing = ctx.entitySize * 2; // TODO: radius as global
  ctx.sGrid.numY = ctx.y_bound / ctx.sGrid.spacing;
  ctx.sGrid.numX = ctx.x_bound / ctx.sGrid.spacing;
  ctx.sGrid.entities = *arr_size_t_create(ctx.sGrid.numX * ctx.sGrid.numY + 1);
  ctx.sGrid.antitiesDense = *arr_size_t_create(ctx.entitiesCount);
  update_spatial(&ctx);
  while (!WindowShouldClose()) {
    ctx.frameTime = GetFrameTime();
    accumulator += ctx.frameTime;
    handle_input(&ctx);

    // velocity increasing over simulation time!?
    Vector2 velocities;
    sum_velocities(&ctx, &velocities);
    float lenght = Vector2Length(velocities);
    printf("(%f,%f) = %f\n", velocities.x, velocities.y, lenght);

    if (!ctx.paused || ctx.step_one_frame) {
      TIME_IT("Update", handle_update(&ctx));
      ctx.step_one_frame = false;
    }

    TIME_IT("Update Spatial", update_spatial(&ctx));

    // RENDERING
    BeginDrawing();
    BeginMode2D(ctx.camera);
    ClearBackground(RAYWHITE);
    DrawText("Hello, World!", 190, 200, 20, LIGHTGRAY);

    TIME_IT("Render", render(&ctx));
    // TIME_IT("Render Spatial Grid", render_spatial_grid(&ctx.sGrid));
    EndMode2D();

    DrawFPS(10, 10);
    // Mouse coordinates
    Vector2 screenPos = GetMousePosition();
    Vector2 worldPos = GetScreenToWorld2D(screenPos, ctx.camera);
    DrawText(TextFormat("Screen: (%.0f, %.0f)", screenPos.x, screenPos.y), 10,
             30, 20, DARKGRAY);
    DrawText(TextFormat("World: (%.1f, %.1f)", worldPos.x, worldPos.y), 10, 50,
             20, DARKGRAY);

    EndDrawing();
  }

  // CLEANUP
  CloseWindow();

  return 0;
}
