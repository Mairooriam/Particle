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
ECS_COMPONENT_DECLARE(c_Camera2D);
void RenderSystem(ecs_iter_t *it) {
  c_Transform *t = ecs_field(it, c_Transform, 0);
  c_Render *r = ecs_field(it, c_Render, 1);

  for (int i = 0; i < it->count; i++) {
    DrawCircle(t[i].pos.x, t[i].pos.y, r[i].renderRadius, r[i].color);
  }
}
void render_entities_ecs(ecs_iter_t *it) {
  c_Transform *_cT = ecs_field(it, c_Transform, 0);
  c_Render *_cR = ecs_field(it, c_Render, 1);
  const c_Camera2D *cC = ecs_singleton_get(it->world, c_Camera2D);
  Camera2D cC2 = {
      .target = {0, 0}, .offset = {400, 300}, .rotation = 0, .zoom = 1};
  for (int i = 0; i < it->count; i++) {
    c_Transform *cT = &_cT[i];
    c_Render *cR = &_cR[i];

    Vector2 screen =
        GetWorldToScreen2D((Vector2){cT->pos.x, cT->pos.y}, cC->camera);

    if (screen.x > 800 || screen.x < 0) {
      continue;
    }

    if (screen.y > 600 || screen.y < 0) {
      continue;
    }
    DrawCircleSector((Vector2){cT->pos.x, cT->pos.y}, cR->renderRadius, 0, 360,
                     16, cR->color);

    // Vector3 n_vel = Vector3Normalize(cT->v);
    // DrawLine(cT->pos.x, cT->pos.y, cT->v.x, cT->v.y, BLACK);
    Vector3 v_relative =
        Vector3Add((Vector3){cT->pos.x, cT->pos.y, 0}, cT->vel);
    DrawLine(cT->pos.x, cT->pos.y, v_relative.x, v_relative.y, BLACK);
    char buf[32];
    snprintf(buf, 32, "%i", i);
    DrawText(buf, cT->pos.x, cT->pos.y, 6.0f, BLACK);
    memset(buf, 0, 32);
    // snprintf(buf, 32, "%zu", cCp1->searchCount);
    // DrawText(buf, cT->pos.x + 10, cT->pos.y + 10, 6.0f, RED);
    // memset(buf, 0, 32);
    // snprintf(buf, 32, "%zu", cCp1->collisionCount);
    // DrawText(buf, cT->pos.x - 10, cT->pos.y + 10, 6.0f, GREEN);
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

  c_Transform t = {.pos = {0.0f, 10.0f, 1.0f},
                   .vel = {0.0f, 0.0f, 0.0f},
                   .acceleration = {0.0f, 0.0f, 0.0f},
                   .restitution = 0.5f};
  c_Render r = {
      .renderRadius = 5.0f,
      .color = RED // or {255, 0, 0, 255}
  };
  c_Collision c = {.radius = 1.0f, .mass = 2.0f, .inverseMass = 3.0f};

  // Create an entity
  register_components(ctx.world);
  ECS_COMPONENT(ctx.world, c_Camera2D);
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
