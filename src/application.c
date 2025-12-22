#include "application.h"
#include "core/components.h"
#include "core/spatial.h"
#include "core/utils.h"
#include "raylib.h"
#include "raymath.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI  // Exclude GDI (prevents Rectangle conflict)
#define NOUSER // Exclude User32 (prevents CloseWindow, ShowCursor conflicts)
#include <windows.h>
#endif

#define NOB_IMPLEMENTATION
#include "core/nob.h"
#include "rlgl.h"
void init_context(ApplicationContext *ctx) {
  ctx->state = APP_STATE_IDLE;
  Camera2D camera = {0};
  camera.zoom = 1.0f;
  ctx->camera = camera;
  ctx->y_bound = 500;
  ctx->x_bound = 500;
  ctx->paused = true;
  ctx->InitFn = entity_init_collision_diagonal;
}

void input(ApplicationContext *ctx) {
  input_state(ctx);
  input_other(ctx);
  // TODO: fix this input mess. just quick setup
  if (ctx->state == APP_STATE_2D) {
    input_mouse_2D(ctx);
  } else {
    input_mouse_3D(ctx);
  }
}
void input_mouse_2D(ApplicationContext *ctx) {
  if (IsCursorHidden()) {
    EnableCursor();
  }
  Camera2D *camera = &ctx->camera;
  if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    Vector2 delta = GetMouseDelta();
    delta = Vector2Scale(delta, -1.0f / camera->zoom);
    camera->target = Vector2Add(camera->target, delta);
  }
  float wheel = GetMouseWheelMove();
  if (wheel != 0) {
    Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), *camera);

    camera->offset = GetMousePosition();
    camera->target = mouseWorldPos;

    float scale = 0.2f * wheel;
    camera->zoom = Clamp(expf(logf(camera->zoom) + scale), 0.125f, 64.0f);
  }
}
void input_mouse_3D(ApplicationContext *ctx) {
  UpdateCamera(&ctx->camera3D, CAMERA_FREE);
  DisableCursor();
  if (IsKeyPressed(KEY_Z))
    ctx->camera3D.target = (Vector3){0.0f, 0.0f, 0.0f};
}
void input_other(ApplicationContext *ctx) {
  if (IsKeyPressed(KEY_R)) {
    // entity_init_collision_not_moving(ctx);
    // entity_init_collision_diagonal(ctx);
    // TODO: LEAK
    SceneData data = {0};
    data.is = ENTITY_INIT_DATAKIND_COUNT;
    data.get.count = ctx->entities->entitiesCount;
    ctx->InitFn(ctx->entities, data);
    ctx->paused = true;
    // entity_init_collision_spatial(ctx);
  }

  if (IsKeyPressed(KEY_SPACE)) {
    ctx->paused = !ctx->paused;
  }

  if (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyDown(KEY_RIGHT)) {
    ctx->step_one_frame = true;
  }
  if (IsKeyPressed(KEY_RIGHT)) {
    ctx->step_one_frame = true;
  }
}

void input_state(ApplicationContext *ctx) {
  int c = GetCharPressed();
  if (c != 0) {
    switch (c) {
    case KEY_ZERO: {
      ctx->state = APP_STATE_IDLE;
    } break;
    case KEY_ONE: {
      ctx->state = APP_STATE_2D;
    } break;
    case KEY_TWO: {
      ctx->state = APP_STATE_3D;
    } break;
    case KEY_THREE: {
    } break;
    case KEY_FOUR: {
    } break;
    case KEY_FIVE: {
    } break;
    case KEY_SIX: {
    } break;
    case KEY_SEVEN: {
    } break;
    case KEY_EIGHT: {
    } break;
    case KEY_NINE: {
    } break;
    }
    printf("char:%c | i:%i\n", c, c);
  }
}
void update(ApplicationContext *ctx) {
  TIME_IT("Reset entity color", color_entities(ctx->entities, GREEN));

  TIME_IT("Update Entities",
          update_entities(ctx->entities, ctx->frameTime, ctx->x_bound,
                          ctx->y_bound, ctx->sGrid));
  TIME_IT("Update Spatial", update_spatial(ctx->sGrid, ctx->entities));
}
void update_entities(Entities *ctx, float frameTime, float x_bound,
                     float y_bound, SpatialGrid *sGrid) {
  for (size_t i = 0; i < ctx->entitiesCount; i++) {
    particle_update_collision_spatial(ctx, i, frameTime, x_bound, y_bound,
                                      sGrid);
  }
}
void collision_simple_reverse(Entities *ctx, size_t idx1, size_t idx2) {
  Component_transform *cTp1 = &ctx->c_transform->items[idx1];
  Component_transform *cTp2 = &ctx->c_transform->items[idx2];
  Component_render *cRp1 = &ctx->c_render->items[idx1];
  Component_render *cRp2 = &ctx->c_render->items[idx2];

  // Old implementation - just reverse
  cTp1->v = Vector2Scale(cTp1->v, -1);
  cTp2->v = Vector2Scale(cTp2->v, -1);
  cRp1->color = (Color){0, 255, 0, 200};
  cRp2->color = (Color){0, 255, 0, 200};
}

// TODO: math this out to understand it.
void collision_elastic_separation(Entities *ctx, size_t idx1, size_t idx2) {
  Component_transform *cTp1 = &ctx->c_transform->items[idx1];
  Component_transform *cTp2 = &ctx->c_transform->items[idx2];
  Component_collision *cCp1 = &ctx->c_collision->items[idx1];
  Component_collision *cCp2 = &ctx->c_collision->items[idx2];
  Component_render *cRp1 = &ctx->c_render->items[idx1];
  Component_render *cRp2 = &ctx->c_render->items[idx2];

  // New implementation - elastic with separation
  Vector2 delta = Vector2Subtract(cTp1->pos, cTp2->pos);
  float distance = Vector2Length(delta);
  Vector2 normal = Vector2Normalize(delta);

  float overlap = (cCp1->radius + cCp2->radius) - distance;
  Vector2 separation = Vector2Scale(normal, overlap / 2.0f);
  cTp1->pos = Vector2Add(cTp1->pos, separation);
  cTp2->pos = Vector2Subtract(cTp2->pos, separation);

  // Swap velocities (for equal masses)
  Vector2 temp = cTp1->v;
  cTp1->v = cTp2->v;
  cTp2->v = temp;

  cRp1->color = (Color){0, 255, 0, 200};
  cRp2->color = (Color){0, 255, 0, 200};
}

void particle_update_collision(Entities *ctx, size_t idx, float frameTime,
                               float x_bound, float y_bound) {
  Component_transform *cTp1 = &ctx->c_transform->items[idx];
  Component_render *cRp1 = &ctx->c_render->items[idx];
  Component_collision *cCp1 = &ctx->c_collision->items[idx];

  float bounds_min = 0.0f;
  cTp1->pos.x = cTp1->pos.x + cTp1->v.x * frameTime;
  cTp1->pos.y = cTp1->pos.y + cTp1->v.y * frameTime;

  for (size_t i = 0; i < ctx->entitiesCount; i++) {
    if (i == idx)
      continue;

    Component_transform *cTp2 = &ctx->c_transform->items[i];
    Component_collision *cCp2 = &ctx->c_collision->items[i];
    //    Component_render *cRp2 = &ctx->c_render->items[i];

    if (CheckCollisionCircles(cTp1->pos, cCp1->radius, cTp2->pos,
                              cCp2->radius)) {
      // TODO: overhead since passing idx, instead of cTp1?
      // collision_simple_reverse(ctx, idx, i);
      collision_elastic_separation(ctx, idx, i);
    }
  }

  // Boundary collision remains the same
  if (cTp1->pos.x < bounds_min) {
    cTp1->pos.x = bounds_min;
    cTp1->v.x = cTp1->v.x * -1;
    cRp1->color = YELLOW;
  } else if (cTp1->pos.x > x_bound) {
    cTp1->pos.x = x_bound;
    cTp1->v.x = cTp1->v.x * -1;
    cRp1->color = BLUE;
  }

  if (cTp1->pos.y < bounds_min) {
    cTp1->pos.y = bounds_min;
    cTp1->v.y = cTp1->v.y * -1;
    cRp1->color = PINK;
  } else if (cTp1->pos.y > y_bound) {
    cTp1->pos.y = y_bound;
    cTp1->v.y = cTp1->v.y * -1;
    cRp1->color = PURPLE;
  }
}
void particle_update_collision_spatial(Entities *ctx, size_t idx,
                                       float frameTime, float x_bound,
                                       float y_bound, SpatialGrid *sGrid) {

  Component_transform *cTp1 = &ctx->c_transform->items[idx];
  Component_render *cRp1 = &ctx->c_render->items[idx];
  Component_collision *cCp1 = &ctx->c_collision->items[idx];
  cCp1->searchCount = 0;
  cCp1->collisionCount = 0;

  float bounds_min = 0.0f;
  cTp1->pos.x = cTp1->pos.x + cTp1->v.x * frameTime;
  cTp1->pos.y = cTp1->pos.y + cTp1->v.y * frameTime;

  // Find which cell this entity is in
  float spacing = sGrid->spacing;
  int xi = (int)(cTp1->pos.x / spacing);
  int yi = (int)(cTp1->pos.y / spacing);

  // printf("starting look around it (%i,%i):%i\n", xi, yi, curcellidx);
  for (int dx = -1; dx <= 1; dx++) {
    for (int dy = -1; dy <= 1; dy++) {
      int nx = xi + dx;
      int ny = yi + dy;

      // Skip if out of bounds
      if (nx < 0 || ny < 0 || nx >= (int)sGrid->numX || ny >= (int)sGrid->numY)
        continue;

      size_t cellIdx = nx * sGrid->numY + ny;

      // Get the range of entities in this cell
      size_t start = sGrid->entities.items[cellIdx];
      size_t end = (cellIdx + 1 < sGrid->entities.capacity)
                       ? sGrid->entities.items[cellIdx + 1]
                       : sGrid->antitiesDense.count;

      // Check collision with all entities in this cell
      for (size_t i = start; i < end; i++) {
        size_t otherIdx = sGrid->antitiesDense.items[i];

        // Skip self-collision
        if (otherIdx == idx)
          continue;

        Component_transform *cTp2 = &ctx->c_transform->items[otherIdx];
        Component_collision *cCp2 = &ctx->c_collision->items[otherIdx];
        Component_render *cRp2 = &ctx->c_render->items[otherIdx];
        if (CheckCollisionCircles(cTp1->pos, cCp1->radius, cTp2->pos,
                                  cCp2->radius)) {
          collision_elastic_separation(ctx, idx, otherIdx);
          cCp1->collisionCount++;
        }
        int r = xi * 50 % 255;
        int g = yi * 50 % 255;
        cRp1->color = (Color){r, g, 0, 255};
        cRp2->color = (Color){r, g, 0, 100};
        cCp1->searchCount++;
      }
    }
  }

  // Boundary collision remains the same
  if (cTp1->pos.x < bounds_min) {
    cTp1->pos.x = bounds_min;
    cTp1->v.x = cTp1->v.x * -1;
    cRp1->color = YELLOW;
  } else if (cTp1->pos.x > x_bound) {
    cTp1->pos.x = x_bound;
    cTp1->v.x = cTp1->v.x * -1;
    cRp1->color = BLUE;
  }

  if (cTp1->pos.y < bounds_min) {
    cTp1->pos.y = bounds_min;
    cTp1->v.y = cTp1->v.y * -1;
    cRp1->color = PINK;
  } else if (cTp1->pos.y > y_bound) {
    cTp1->pos.y = y_bound;
    cTp1->v.y = cTp1->v.y * -1;
    cRp1->color = PURPLE;
  }
}

// ---------------------------------------------
// RENDERING
// ---------------------------------------------
void render(ApplicationContext *ctx) {

  DrawRectangleLines(0, 0, ctx->x_bound, ctx->y_bound, BLACK);

  TIME_IT("Render Spatial Grid", render_spatial_grid(ctx->sGrid, ctx->camera));
  TIME_IT("Render Entities", render_entities(ctx->entities, ctx->camera));
}

void render_entities(Entities *ctx, Camera2D camera) {
  for (size_t i = 0; i < ctx->entitiesCount; i++) {

    Component_transform *cT = &ctx->c_transform->items[i];

    Vector2 screen = GetWorldToScreen2D(cT->pos, camera);

    if (screen.x > 800 || screen.x < 0) {
      continue;
    }

    if (screen.y > 600 || screen.y < 0) {
      continue;
    }

    Component_render *cR = &ctx->c_render->items[i];
    Component_collision *cCp1 = &ctx->c_collision->items[i]; // TODO: temp
                                                             // debug;

    DrawCircleSector(cT->pos, cR->renderRadius, 0, 360, 16, cR->color);

    // Vector2 n_vel = Vector2Normalize(cT->v);
    // DrawLine(cT->pos.x, cT->pos.y, cT->v.x, cT->v.y, BLACK);
    Vector2 v_relative = Vector2Add(cT->pos, cT->v);
    DrawLine(cT->pos.x, cT->pos.y, v_relative.x, v_relative.y, BLACK);
    char buf[32];
    snprintf(buf, 32, "%zu", i);
    DrawText(buf, cT->pos.x, cT->pos.y, 6.0f, BLACK);
    memset(buf, 0, 32);
    snprintf(buf, 32, "%zu", cCp1->searchCount);
    DrawText(buf, cT->pos.x + 10, cT->pos.y + 10, 6.0f, RED);
    memset(buf, 0, 32);
    snprintf(buf, 32, "%zu", cCp1->collisionCount);
    DrawText(buf, cT->pos.x - 10, cT->pos.y + 10, 6.0f, GREEN);
  }
}
void render_entities_3D(Entities *e, Camera camera3D) {
  for (size_t i = 0; i < e->entitiesCount; i++) {
    Component_render *cRp1 = &e->c_render->items[i];
    Component_transform *cTp1 = &e->c_transform->items[i];

    rlDisableBackfaceCulling();
    rlPushMatrix();
    rlTranslatef(cTp1->pos.x, cTp1->pos.y, 0.0f);
    rlRotatef(-90, 1, 0, 0);
    DrawModel(*cRp1->model, (Vector3){0, 0, 0}, 2.0f, WHITE);
    rlPopMatrix();
    rlEnableBackfaceCulling();
  }
}

void render_info(ApplicationContext *ctx) {
  DrawFPS(10, 10);
  // Mouse coordinates
  Vector2 screenPos = GetMousePosition();
  Vector2 worldPos = GetScreenToWorld2D(screenPos, ctx->camera);
  DrawText(TextFormat("Screen: (%.0f, %.0f)", screenPos.x, screenPos.y), 10, 30,
           20, DARKGRAY);
  DrawText(TextFormat("World: (%.1f, %.1f)", worldPos.x, worldPos.y), 10, 50,
           20, DARKGRAY);
}

void render_spatial_grid(SpatialGrid *sGrid, Camera2D camera) {
  // spatial grid - draw cells as rectangles
  // TODO: -1 on numx etc fixes the problem but why? might cause porlbmes
  for (size_t x = 0; x <= sGrid->numX - 1; x++) {
    for (size_t y = 0; y <= sGrid->numY - 1; y++) {
      float posX = x * sGrid->spacing;
      float posY = y * sGrid->spacing;
      Rectangle rec = (Rectangle){posX, posY, sGrid->spacing, sGrid->spacing};

      Vector2 screen = GetWorldToScreen2D((Vector2){posX, posY}, camera);
      if (screen.x > 800 || screen.x < 0) {
        continue;
      }

      if (screen.y > 600 || screen.y < 0) {
        continue;
      }
      // Calculate cell index
      size_t idx = x * sGrid->numY + y;

      // Color based on whether the cell has entities
      //    Color cellColor;
      // if (idx < sGrid->entities.capacity &&
      //     sGrid->entities.items[idx].count > 0) {
      //   cellColor = (Color){255, 100, 100, 100}; // Red tint if occupied
      // } else {
      //   cellColor = (Color){200, 200, 200, 50}; // Light gray if empty
      // }
      char buf[10];
      snprintf(buf, 10, "%zu", idx);

      DrawText(buf, rec.x + (rec.width / 2), rec.y + (rec.height / 4), 14.0f,
               BLACK);
      int r = x * 50 % 255;
      int g = y * 50 % 255;
      DrawRectangleRec(rec, (Color){r, g, 10, 100});
      DrawRectangleLinesEx(rec, 1.0f, (Color){150, 150, 150, 255});
    }
  }

  // Flatten overlay
  Vector2 offset = {-100, -100};
  size_t size = 25.0f;
  for (size_t i = 0; i < sGrid->entities.capacity; i++) {
    Rectangle rec =
        (Rectangle){offset.x + i * (size + 5), offset.y, size, size};

    Vector2 screen = GetWorldToScreen2D((Vector2){rec.x, rec.y}, camera);
    if (screen.x > 800 || screen.x < 0 || screen.y > 600 || screen.y < 0) {
      continue;
    }

    size_t val = sGrid->entities.items[i];
    char buf[10];
    snprintf(buf, 10, "%zu", val);
    if (val != 0) {
      DrawRectangleLinesEx(rec, 2.0f, PINK);
    } else {
      DrawRectangleLinesEx(rec, 2.0f, BLACK);
    }
    DrawText(buf, rec.x + (rec.width / 2), rec.y + (rec.height / 4), 14.0f,
             BLACK);
    memset(buf, 0, sizeof(buf));
    snprintf(buf, 10, "%zu", i);
    DrawText(buf, rec.x + rec.width / 2, rec.y - 14, 14.0f, BLACK);

    // for (size_t j = 0; j < sGrid->entities.items[i].count; j++) {
    //   Rectangle rec = (Rectangle){offset.x + i * (size + 5),
    //                               offset.y + (j + 1) * (size + 5), size,
    //                               size};
    //   size_t val = sGrid->entities.items[i].items[j];
    //   char buf2[10];
    //   snprintf(buf2, 10, "%zu", val);
    //   DrawText(buf2, rec.x + (rec.width / 2), rec.y + (rec.height /
    //   4), 14.0f,
    //            BLACK);
    //
    //   DrawRectangleLinesEx(rec, 3.0f, RED);
    // }
  }

  // Flatten for dense
  for (size_t i = 0; i < sGrid->antitiesDense.capacity; i++) {
    Rectangle rec =
        (Rectangle){offset.x + i * (size + 5), offset.y * 2, size, size};
    Vector2 screen = GetWorldToScreen2D((Vector2){rec.x, rec.y}, camera);
    if (screen.x > 800 || screen.x < 0 || screen.y > 600 || screen.y < 0) {
      continue;
    }
    DrawRectangleLinesEx(rec, 2.0f, PINK);
    size_t val = sGrid->antitiesDense.items[i];
    char buf3[10];
    snprintf(buf3, 10, "%zu", val);
    DrawText(buf3, rec.x + (rec.width / 2), rec.y + (rec.height / 4), 14.0f,
             BLACK);
  }
}
// ---------------------------------------------
//
// ---------------------------------------------
