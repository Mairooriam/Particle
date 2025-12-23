#include "application.h"
#include "core/components.h"
#include "core/spatial.h"
#include "core/utils.h"
#include "raylib.h"
#include "rlgl.h"
#include <stdio.h>

#ifdef ENABLE_TIMING
TimingEntry g_timing_entries[32];
int g_timing_count = 0;
double g_timing_accumulated_time = 0.0;
double g_timing_print_interval = 1.0;
#endif

int main(void) {

  int bounds_x = 4000 * 3;
  int bounds_y = 4000 * 3;

  // Context init
  size_t entityCount = 2;
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
  // init_context(&ctx, entityCount);

  // Initialize the window
  InitWindow(800, 600, "Raylib Hello World");
  SetTargetFPS(60);

  // scene_multiple(&ctx, data);
  update_spatial(sGrid, entities);

  TIMING_SET_INTERVAL(1.0); // Print every 1 second
  // TIMING_SET_INTERVAL(5.0);  // Or every 5 seconds
  // TIMING_SET_INTERVAL(10.0); // Or every 10 secon
  {
    Camera camera3D = {{50.0f, 50.0f, 50.0f},
                       {0.0f, 0.0f, 0.0f},
                       {0.0f, 1.0f, 0.0f},
                       45.0f,
                       0};
    ctx.camera3D = camera3D;
  }

  Mesh mesh = mesh_generate_circle(32);
  Model model = LoadModelFromMesh(mesh);
  Image checked = GenImageChecked(2, 2, 1, 1, RED, GREEN);
  Texture2D texture = LoadTextureFromImage(checked);
  Entities *entities2 = entities_create(50000);
  UnloadImage(checked);

  model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
  for (size_t i = 0; i < entities2->entitiesCount; i++) {
    // Component_render *cRp1 = &entities2->c_render->items[i];
    // cRp1->model = &model;
    Component_transform *cTp1 = &entities2->c_transform->items[i];
    cTp1->pos.y = i + 1;
    cTp1->pos.x = i * 3 + 1;
    cTp1->pos.z = (float)GetRandomValue(0, 50) / 10.0f;

    cTp1->v.x = (float)GetRandomValue(-500, 500) / 10.0f;
    cTp1->v.y = (float)GetRandomValue(-500, 500) / 10.0f;
  }

  while (!WindowShouldClose()) {
    TIMING_FRAME_BEGIN();

    // for (size_t i = 0; i < entities2->entitiesCount; i++) {
    //   Component_transform *cTp1 = &entities2->c_transform->items[i];
    //   printf("pX:%f,pY:%f,pZ:%f\n", cTp1->pos.x, cTp1->pos.y, cTp1->pos.z);
    //   printf("vX:%f,vY:%f,vZ:%f\n", cTp1->v.x, cTp1->v.y, cTp1->v.z);
    // }

    ctx.frameTime = GetFrameTime();

    // LOGGING
    // TIME_IT("Log_Velocities", log_velocities(ctx.entities));

    // INPUT
    TIME_IT("Input", input(&ctx));

    // UPDATE
    if (!ctx.paused || ctx.step_one_frame) {
      TIME_IT("Update 3d", update_entities_3D(entities2, ctx.frameTime,
                                              ctx.x_bound, ctx.y_bound, NULL));
      TIME_IT("Update total", update(&ctx));
      ctx.step_one_frame = false;
    }

    // RENDERING
    BeginDrawing();
    ClearBackground(BLACK);
    BeginMode3D(ctx.camera3D);
    DrawGrid(10, 1.0);
    DrawPlane((Vector3){0, 0, 0}, (Vector2){100, 100}, GRAY);
    DrawSphere((Vector3){50, 0, 50}, 5, RED);
    DrawSphere((Vector3){-50, 0, -50}, 5, BLUE);
    DrawSphere((Vector3){-50, 0, 50}, 5, GREEN);
    DrawSphere((Vector3){50, 0, -50}, 5, YELLOW);

    // rlDisableBackfaceCulling();
    // rlPushMatrix();
    // rlRotatef(-90, 1, 0, 0);
    // DrawModel(model, (Vector3){0, 0, 0}, 2.0f, WHITE);
    // rlPopMatrix();
    // rlEnableBackfaceCulling();

    TIME_IT("Render 3D", render_entities_3D(entities2, ctx.camera3D));
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
