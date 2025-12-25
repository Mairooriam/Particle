#include "application.h"
#include "core/components.h"
#include "core/spatial.h"
#include "core/utils.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
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
  size_t entityCount = 10000;

  Entities *entities = entities_create(entityCount);
  SpatialGrid *sGrid =
      SpatialGrid_create(bounds_x, bounds_y, 50.0f, entities->entitiesCount);

  ApplicationContext ctx = {0};
  ctx.entities = entities;
  ctx.sGrid = sGrid;
  ctx.x_bound = bounds_x;
  ctx.y_bound = bounds_y;
  Camera2D camera2d = {0};
  camera2d.zoom = 1.0f;
  ctx.camera = camera2d;
  ctx.paused = true;
  ctx.state = APP_STATE_2D;

  SceneData data = {0};
  data.is = ENTITY_INIT_DATAKIND_FULL;
  data.get.full.count = entityCount;
  data.get.full.entitySize = 25.0f;
  data.get.full.boundsX = bounds_x;
  data.get.full.boundsY = bounds_y;

  entity_init_collision_diagonal(ctx.entities, data);

  // Initialize the window
  InitWindow(800, 600, "Raylib Hello World");
  SetTargetFPS(60);

  update_spatial(sGrid, entities);

  TIMING_SET_INTERVAL(1.0); // Print every 1 second
  // TIMING_SET_INTERVAL(5.0);  // Or every 5 seconds
  // TIMING_SET_INTERVAL(10.0); // Or every 10 secon
  {
    Camera camera3D = {{60.0f, 60.0f, 60.0f},
                       {0.0f, 0.0f, 0.0f},
                       {0.0f, 1.0f, 0.0f},
                       60.0f,
                       0};
    ctx.camera3D = camera3D;
    ctx.camera3D.projection = CAMERA_PERSPECTIVE;
  }

  // ---------------
  // ENTITIES 3D
  // ---------------

  Entities *entities2 = entities_create(500);

  for (size_t i = 0; i < entities2->entitiesCount; i++) {
    Component_transform *cTp1 = &entities2->c_transform->items[i];
    cTp1->pos.y = i + 1;
    cTp1->pos.x = i * 3 + 1;
    cTp1->pos.z = (float)GetRandomValue(0, 500) / 10.0f;

    cTp1->v.x = (float)GetRandomValue(-500, 500) / 10.0f;
    cTp1->v.y = (float)GetRandomValue(-500, 500) / 10.0f;
  }
  Mesh mesh = GenMeshCube(1.0f, 1.0f, 1.0f);

  Matrix *transforms =
      (Matrix *)RL_CALLOC(entities2->entitiesCount, sizeof(Matrix));
  Shader shader = LoadShader("lighting_instancing.vs", "lighting.fs");
  Material matInstances = LoadMaterialDefault();
  init_instanced_draw(&shader, transforms, entities2, &matInstances);

  // ---------------
  // ENTITIES 3D
  // ---------------

  Material matDefault = LoadMaterialDefault();
  matDefault.maps[MATERIAL_MAP_DIFFUSE].color = BLUE;

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
      TIME_IT("Update 3d",
              update_entities_3D(entities2, ctx.frameTime, NULL, transforms));
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

    TIME_IT("Render 3D", render_entities_3D(mesh, matInstances, transforms,
                                            entities2->entitiesCount));
    DrawMesh(mesh, matDefault, MatrixTranslate(-10.0f, 0.0f, 0.0f));
    DrawMesh(mesh, matDefault, MatrixTranslate(10.0f, 0.0f, 0.0f));
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
