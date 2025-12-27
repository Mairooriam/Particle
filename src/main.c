#include "application.h"
#include "core/components.h"
#include "core/spatial.h"
#include "core/utils.h"
#include "entities.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <flecs.h>
#include <stdio.h>
#define GLSL_VERSION 330
#ifdef ENABLE_TIMING
TimingEntry g_timing_entries[32];
int g_timing_count = 0;
double g_timing_accumulated_time = 0.0;
double g_timing_print_interval = 1.0;
#endif
#define MAX_INSTANCES 100
void RenderSystem(ecs_iter_t *it) {
  c_Transform *t = ecs_field(it, c_Transform, 0);
  c_Render *r = ecs_field(it, c_Render, 1);

  for (int i = 0; i < it->count; i++) {
    t[i].pos = Vector3Add(t[i].pos, (Vector3){10, 10, 0});
    DrawCircle(t[i].pos.x, t[i].pos.y, r[i].renderRadius, r[i].color);
  }
}
void render_entities_ecs(ecs_iter_t *it) {
  c_Transform *_cT = ecs_field(it, c_Transform, 0);
  c_Render *_cR = ecs_field(it, c_Render, 1);
  const c_Camera2D *cC = ecs_singleton_get(it->world, c_Camera2D);

  for (int i = 0; i < it->count; i++) {
    c_Transform *cT = &_cT[i];
    c_Render *cR = &_cR[i];

    cT->vel = Vector3Add(cT->vel, Vector3Scale(cT->acceleration, it->delta_time));
    cT->pos = Vector3Add(cT->pos, Vector3Scale(cT->vel, it->delta_time));

    if (cT->pos.x > 800) {
      cT->pos.x = 800;
      cT->vel.x = -cT->vel.x; 
    } else if (cT->pos.x < 0) {
      cT->pos.x = 0;
      cT->vel.x = -cT->vel.x; 
    }
    if (cT->pos.y > 600) {
      cT->pos.y = 600;
      cT->vel.y = -cT->vel.y; 
    } else if (cT->pos.y < 0) {
      cT->pos.y = 0;
      cT->vel.y = -cT->vel.y; 
    }
    Vector2 screen = GetWorldToScreen2D((Vector2){cT->pos.x, cT->pos.y}, cC->camera);
    if (screen.x > 800 || screen.x < 0 || screen.y > 600 || screen.y < 0) {
      continue;
    }

    Vector3 v_relative =
        Vector3Add((Vector3){cT->pos.x, cT->pos.y, 0}, cT->vel);
    DrawLine(cT->pos.x, cT->pos.y, v_relative.x, v_relative.y, BLACK);

    char buf[32];
    snprintf(buf, 32, "%i", i);
    DrawText(buf, cT->pos.x, cT->pos.y, 6.0f, BLACK);

    memset(buf, 0, 32);
    float speed = Vector3Length(cT->vel); // Speed = length of velocity vector
    snprintf(buf, 32, "%.2f", speed);     // Format to 2 decimal places
    DrawText(buf, cT->pos.x + 10, cT->pos.y + 10, 6.0f,
             BLUE); // Position offset, blue color

    DrawCircleSector((Vector2){cT->pos.x, cT->pos.y}, cR->renderRadius, 0, 360,
                     16, cR->color);
  }
}

int main(void) {
  // Initialize the window
  //
  InitWindow(800, 600, "Raylib Hello World");
  SetTargetFPS(60);

  int bounds_x = 800;
  int bounds_y = 600;

  // -----------------------
  // ENTITY COUNT 2D
  // -------------------------

  ApplicationContext ctx = {0};
  init_context(&ctx, bounds_x, bounds_y);
  TIMING_SET_INTERVAL(1.0); // Print every 1 second

  c_Transform t = {.pos = {100.0f, 100.0f, 0.0f},
                   .vel = {-50.0f, -50.0f, 0.0f},
                   .acceleration = {0.0f, 0.0f, 0.0f},
                   .restitution = 0.5f};
  c_Render r = {
      .renderRadius = 5.0f,
      .color = RED // or {255, 0, 0, 255}
  };
  c_Collision c = {.radius = 5.0f, .mass = 2.0f, .inverseMass = 3.0f};

  // Create an entity
  register_components(ctx.world);
  ecs_entity_t entity = create_entity(ctx.world, t, c, r);
  printf("Created entity: %llu\n", entity);
  ECS_SYSTEM(ctx.world, render_entities_ecs, EcsOnUpdate, c_Transform,
             c_Render);

  while (!WindowShouldClose()) {
    TIMING_FRAME_BEGIN();
    UpdateCamera(&ctx.camera3D, CAMERA_ORBITAL);

    ctx.frameTime = GetFrameTime();

    // LOGGING
    /* TIME_IT("Log_Velocities", log_velocities(ctx.entities)); */

    // INPUT
    TIME_IT("Input", input(&ctx));

    // UPDATE
    if (!ctx.paused || ctx.step_one_frame) {
      TIME_IT("Update total", update(&ctx));
      ctx.step_one_frame = false;
    }
    ecs_singleton_set(ctx.world, c_Camera2D, {.camera = ctx.camera});

    // RENDERING
    BeginDrawing();
    ClearBackground(GRAY);
    BeginMode3D(ctx.camera3D);

    EndMode3D();

    BeginMode2D(ctx.camera);
    DrawRectangle(0, 0, 800, 600, PINK);
    ecs_progress(ctx.world, ctx.frameTime);

    DrawText("Hello, World!", 190, 200, 20, LIGHTGRAY);
    EndMode2D();

    render_info(&ctx);

    TIMING_FRAME_END(ctx.frameTime);

    char camInfo[128];
    snprintf(camInfo, sizeof(camInfo), "Camera - Pos:(%.0f, %.0f) Zoom:%.2f",
             ctx.camera.target.x, ctx.camera.target.y, ctx.camera.zoom);
    DrawText(camInfo, 10, 150, 16, DARKGRAY);

    EndDrawing();
  }

  // CLEANUP
  CloseWindow();

  return 0;
}
