#include "application.h"
#include "core/components.h"
#include "core/spatial.h"
#include "core/utils.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "stdbool.h"
#include <stdio.h>
#define GLSL_VERSION 330
#ifdef ENABLE_TIMING
TimingEntry g_timing_entries[32];
int g_timing_count = 0;
double g_timing_accumulated_time = 0.0;
double g_timing_print_interval = 1.0;
#endif
#define MAX_INSTANCES 100
int main(void) {

  int bounds_x = 10000;
  int bounds_y = 10000;

  // -----------------------
  // ENTITY COUNT 2D
  // -------------------------

  ApplicationContext ctx = {0};
  {
    size_t entityCount = 1000000;

    Entities entities;
    Entities_init(&entities, entityCount);

    SpatialGrid *sGrid =
        SpatialGrid_create(bounds_x, bounds_y, 50.0f, entities.capacity);
    ctx.entities = &entities;
    ctx.sGrid = sGrid;
    ctx.x_bound = bounds_x;
    ctx.y_bound = bounds_y;
    Camera2D camera2d = {0};
    camera2d.zoom = 1.0f;
    ctx.camera = camera2d;
    ctx.paused = true;
    ctx.state = APP_STATE_2D;
    Camera camera3D = {{60.0f, 60.0f, 60.0f},
                       {0.0f, 0.0f, 0.0f},
                       {0.0f, 1.0f, 0.0f},
                       60.0f,
                       0};
    ctx.camera3D = camera3D;
    ctx.camera3D.projection = CAMERA_PERSPECTIVE;
    ctx.spawnerInitalized = false;
  }

  InitWindow(800, 600, "Raylib Hello World");
  SetTargetFPS(60);

  update_spatial(ctx.sGrid, ctx.entities);

  TIMING_SET_INTERVAL(1.0); // Print every 1 second
  // Entity e = {0};
  // entity_add(ctx.entities, entity_create_spawner_entity());
  Entity e =
      entity_create_physics_particle((Vector3){100, 50, 0}, (Vector3){0, 0, 0});
  Entity e2 = entity_create_physics_particle((Vector3){100, 100, 0},
                                             (Vector3){0, 0, 0});

  c_Spring cSp1 = {0};
  cSp1.springConstat = 1.0f;
  cSp1.restLenght = 2.0f;
  cSp1.parent = 1;
  e.c_spring = cSp1;
  FLAG_SET(e.flags, ENTITY_FLAG_SPRING);

  entity_add(ctx.entities, e);
  entity_add(ctx.entities, e2);

  while (!WindowShouldClose()) {
    TIMING_FRAME_BEGIN();
    UpdateCamera(&ctx.camera3D, CAMERA_ORBITAL);

    ctx.frameTime = GetFrameTime();

    // INPUT
    TIME_IT("Input", input(&ctx));

    // UPDATE
    if (!ctx.paused || ctx.step_one_frame) {
      // TIME_IT("Update 3d",
      //         update_entities_3D(entities2, ctx.frameTime, NULL,
      //         transforms));
      TIME_IT("Update total", update(&ctx));
      ctx.step_one_frame = false;
    }

    // RENDERING
    BeginDrawing();
    ClearBackground(GRAY);
    BeginMode3D(ctx.camera3D);
    DrawGrid(10, 1.0);
    // DrawPlane((Vector3){0, 0, 0}, (Vector2){100, 100}, GRAY);
    DrawSphere((Vector3){50, 0, 50}, 5, RED);
    DrawSphere((Vector3){-50, 0, -50}, 5, BLUE);
    DrawSphere((Vector3){-50, 0, 50}, 5, GREEN);
    DrawSphere((Vector3){50, 0, -50}, 5, YELLOW);

    // TIME_IT("Render 3D", render_entities_3D(mesh, matInstances, transforms,
    //                                         entities2->entitiesCount));
    // DrawMesh(mesh, matDefault, MatrixTranslate(-10.0f, 0.0f, 0.0f));
    // DrawMesh(mesh, matDefault, MatrixTranslate(10.0f, 0.0f, 0.0f));
    EndMode3D();

    BeginMode2D(ctx.camera);
    DrawText("Hello, World!", 190, 200, 20, LIGHTGRAY);
    TIME_IT("Render total", render(&ctx));
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
