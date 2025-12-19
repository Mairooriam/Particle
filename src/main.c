

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

  ctx.particles = Particles_create(16);
  for (size_t i = 0; i < ctx.particles->capacity; i++) {
    ctx.particles->items[i].pos.x = i;
    ctx.particles->items[i].pos.y = i * 2;
    ctx.particles->count++;
  }

  // Initialize the window
  InitWindow(ctx.window.width, ctx.window.height, "Raylib Hello World");
  SetTargetFPS(60);
  while (!WindowShouldClose()) {
    handle_input(&ctx);
    handle_update(&ctx);
    // RENDERING
    BeginDrawing();
    BeginMode2D(camera);

    ClearBackground(RAYWHITE);
    DrawText("Hello, World!", 190, 200, 20, LIGHTGRAY);

    render(&ctx);
    EndMode2D();

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
