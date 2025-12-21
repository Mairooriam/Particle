#include "spatial.h"
#include "raylib.h"
#include "raymath.h"
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
#include "nob.h"
#include "rlgl.h"
void render(SpatialContext *ctx) {

  DrawRectangleLines(0, 0, ctx->x_bound, ctx->y_bound, BLACK);

  for (size_t i = 0; i < ctx->entitiesCount; i++) {
    Component_transform *cT = &ctx->c_transform->items[i];
    Component_render *cR = &ctx->c_render->items[i];

    DrawCircleSector(cT->pos, cR->renderRadius, 0, 360, 8, cR->color);

    // Vector2 n_vel = Vector2Normalize(cT->v);
    // DrawLine(cT->pos.x, cT->pos.y, cT->v.x, cT->v.y, BLACK);
    Vector2 v_relative = Vector2Add(cT->pos, cT->v);
    DrawLine(cT->pos.x, cT->pos.y, v_relative.x, v_relative.y, BLACK);
    char buf[10];
    snprintf(buf, 10, "%zu", i);
    DrawText(buf, cT->pos.x, cT->pos.y, 6.0f, BLACK);
  }
}

void handle_input(SpatialContext *ctx) {
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

  if (IsKeyPressed(KEY_R)) {
    // init_collision_not_moving(ctx);
    init_collision_diagonal(ctx);
    // TODO: LEAK
    // init(ctx, 5000);
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

void handle_update(SpatialContext *ctx) {

  for (size_t i = 0; i < ctx->entitiesCount; i++) {
    particle_update_collision_spatial(ctx, i);
  }
}
void collision_simple_reverse(SpatialContext *ctx, size_t idx1, size_t idx2) {
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
void collision_elastic_separation(SpatialContext *ctx, size_t idx1,
                                  size_t idx2) {
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

void particle_update_collision(SpatialContext *ctx, size_t idx) {
  Component_transform *cTp1 = &ctx->c_transform->items[idx];
  Component_render *cRp1 = &ctx->c_render->items[idx];
  Component_collision *cCp1 = &ctx->c_collision->items[idx];

  float bounds_min = 0.0f;
  cTp1->pos.x = cTp1->pos.x + cTp1->v.x * ctx->frameTime;
  cTp1->pos.y = cTp1->pos.y + cTp1->v.y * ctx->frameTime;

  for (size_t i = 0; i < ctx->entitiesCount; i++) {
    if (i == idx)
      continue;

    Component_transform *cTp2 = &ctx->c_transform->items[i];
    Component_collision *cCp2 = &ctx->c_collision->items[i];
    Component_render *cRp2 = &ctx->c_render->items[i];

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
  } else if (cTp1->pos.x > ctx->x_bound) {
    cTp1->pos.x = ctx->x_bound;
    cTp1->v.x = cTp1->v.x * -1;
    cRp1->color = BLUE;
  }

  if (cTp1->pos.y < bounds_min) {
    cTp1->pos.y = bounds_min;
    cTp1->v.y = cTp1->v.y * -1;
    cRp1->color = PINK;
  } else if (cTp1->pos.y > ctx->y_bound) {
    cTp1->pos.y = ctx->y_bound;
    cTp1->v.y = cTp1->v.y * -1;
    cRp1->color = PURPLE;
  }
}
void particle_update_collision_spatial(SpatialContext *ctx, size_t idx) {
  Component_transform *cTp1 = &ctx->c_transform->items[idx];
  Component_render *cRp1 = &ctx->c_render->items[idx];
  Component_collision *cCp1 = &ctx->c_collision->items[idx];

  float bounds_min = 0.0f;
  cTp1->pos.x = cTp1->pos.x + cTp1->v.x * ctx->frameTime;
  cTp1->pos.y = cTp1->pos.y + cTp1->v.y * ctx->frameTime;

  // Find which cell this entity is in
  float spacing = ctx->sGrid.spacing;
  int xi = (int)(cTp1->pos.x / spacing);
  int yi = (int)(cTp1->pos.y / spacing);

  int curcellidx = xi * ctx->sGrid.numY + yi;
  // printf("starting look around it (%i,%i):%i\n", xi, yi, curcellidx);
  for (int dx = -1; dx <= 1; dx++) {
    for (int dy = -1; dy <= 1; dy++) {
      int nx = xi + dx;
      int ny = yi + dy;

      // Skip if out of bounds
      if (nx < 0 || ny < 0 || nx >= (int)ctx->sGrid.numX ||
          ny >= (int)ctx->sGrid.numY)
        continue;

      size_t cellIdx = nx * ctx->sGrid.numY + ny;

      // Get the range of entities in this cell
      size_t start = ctx->sGrid.entities.items[cellIdx];
      size_t end = (cellIdx + 1 < ctx->sGrid.entities.capacity)
                       ? ctx->sGrid.entities.items[cellIdx + 1]
                       : ctx->sGrid.antitiesDense.count;

      // Check collision with all entities in this cell
      for (size_t i = start; i < end; i++) {
        size_t otherIdx = ctx->sGrid.antitiesDense.items[i];

        // Skip self-collision
        if (otherIdx == idx)
          continue;

        Component_transform *cTp2 = &ctx->c_transform->items[otherIdx];
        Component_collision *cCp2 = &ctx->c_collision->items[otherIdx];

        if (CheckCollisionCircles(cTp1->pos, cCp1->radius, cTp2->pos,
                                  cCp2->radius)) {
          collision_elastic_separation(ctx, idx, otherIdx);
        }
      }
    }
  }

  // Boundary collision remains the same
  if (cTp1->pos.x < bounds_min) {
    cTp1->pos.x = bounds_min;
    cTp1->v.x = cTp1->v.x * -1;
    cRp1->color = YELLOW;
  } else if (cTp1->pos.x > ctx->x_bound) {
    cTp1->pos.x = ctx->x_bound;
    cTp1->v.x = cTp1->v.x * -1;
    cRp1->color = BLUE;
  }

  if (cTp1->pos.y < bounds_min) {
    cTp1->pos.y = bounds_min;
    cTp1->v.y = cTp1->v.y * -1;
    cRp1->color = PINK;
  } else if (cTp1->pos.y > ctx->y_bound) {
    cTp1->pos.y = ctx->y_bound;
    cTp1->v.y = cTp1->v.y * -1;
    cRp1->color = PURPLE;
  }
}
void init(SpatialContext *ctx, size_t count) {
  ctx->entitiesCount = count;

  if (ctx->c_transform)
    Components_transform_free(ctx->c_transform);
  if (ctx->c_render)
    Components_render_free(ctx->c_render);
  if (ctx->c_collision)
    Components_collision_free(ctx->c_collision);

  ctx->c_transform = Components_transform_create(count);
  ctx->c_render = Components_render_create(count);
  ctx->c_collision = Components_collision_create(count);

  for (size_t i = 0; i < count; i++) {
    // TRANSFROM
    ctx->c_transform->items[i].pos =
        (Vector2){.x = (float)(i % ctx->window.width),
                  .y = (float)(i % ctx->window.height)};
    ctx->c_transform->items[i].v =
        (Vector2){.x = (float)(i % 5) * 10.0f - 20.0f,
                  .y = (float)(i % 3) * 15.0f - 15.0f};
    ctx->c_transform->count++;

    // RENDER
    ctx->c_render->items[i].renderRadius = ctx->entitySize;
    ctx->c_render->items[i].color = (Color){255, 0, 0, 200};
    ctx->c_render->count++;

    // COLLISION
    ctx->c_collision->items[i].radius = ctx->entitySize;
    ctx->c_collision->items[i].mass = 5.0f;
    ctx->c_collision->count++;
  }
}

void init_collision_head_on(SpatialContext *ctx) {
  size_t count = 3;
  ctx->entitiesCount = count;
  ctx->c_transform = Components_transform_create(count);
  ctx->c_render = Components_render_create(count);
  ctx->c_collision = Components_collision_create(count);
  for (size_t i = 0; i < count; i++) {
    // TRANSFROM
    if (i == 0) {
      // Left particle - moving right
      ctx->c_transform->items[i].pos = (Vector2){.x = 0, .y = 150};
      ctx->c_transform->items[i].v = (Vector2){.x = 50, .y = 0};
    } else if (i == 1) {
      // middle particle - stationary
      ctx->c_transform->items[i].pos = (Vector2){.x = 150, .y = 150};
      ctx->c_transform->items[i].v = (Vector2){.x = 0, .y = 0};
    } else {
      // right particle - moving left
      ctx->c_transform->items[i].pos = (Vector2){.x = 200, .y = 150};
      ctx->c_transform->items[i].v = (Vector2){.x = -50, .y = 0};
    }
    ctx->c_transform->count++;

    // render
    ctx->c_render->items[i].renderRadius = ctx->entitySize;
    ctx->c_render->items[i].color = (Color){255, 0, 0, 200};
    ctx->c_render->count++;

    // collision
    ctx->c_collision->items[i].radius = ctx->entitySize;
    ctx->c_collision->items[i].mass = 5.0f;
    ctx->c_collision->count++;
  }
}

void init_collision_not_moving(SpatialContext *ctx) {
  size_t count = 3;
  ctx->entitiesCount = count;
  ctx->c_transform = Components_transform_create(count);
  ctx->c_render = Components_render_create(count);
  ctx->c_collision = Components_collision_create(count);
  for (size_t i = 0; i < count; i++) {
    // TRANSFROM
    if (i == 0) {
      // Left particle - moving right
      ctx->c_transform->items[i].pos = (Vector2){.x = 145, .y = 150};
      ctx->c_transform->items[i].v = (Vector2){.x = 0, .y = 0};
    } else if (i == 1) {
      // middle particle - stationary
      ctx->c_transform->items[i].pos = (Vector2){.x = 150, .y = 150};
      ctx->c_transform->items[i].v = (Vector2){.x = 0, .y = 0};
    } else {
      // right particle - moving left
      ctx->c_transform->items[i].pos = (Vector2){.x = 155, .y = 150};
      ctx->c_transform->items[i].v = (Vector2){.x = 0, .y = 0};
    }
    ctx->c_transform->count++;

    // render
    ctx->c_render->items[i].renderRadius = ctx->entitySize;
    ctx->c_render->items[i].color = (Color){255, 0, 0, 200};
    ctx->c_render->count++;

    // collision
    ctx->c_collision->items[i].radius = ctx->entitySize;
    ctx->c_collision->items[i].mass = 5.0f;
    ctx->c_collision->count++;
  }
}

void appLoopMain(SpatialContext *ctx) {
  handle_input(ctx);

  if (!ctx->paused || ctx->step_one_frame) {
    TIME_IT("Update", handle_update(ctx));
    ctx->step_one_frame = false;
  }

  // RENDERING
  BeginDrawing();
  BeginMode2D(ctx->camera);
  ClearBackground(RAYWHITE);
  DrawText("Hello, World!", 190, 200, 20, LIGHTGRAY);
  TIME_IT("Render", render(ctx));
  EndMode2D();

  DrawFPS(10, 10);
  EndDrawing();
}
void init_collision_diagonal(SpatialContext *ctx) {
  size_t count = 6;
  ctx->entitiesCount = count;
  ctx->c_transform = Components_transform_create(count);
  ctx->c_render = Components_render_create(count);
  ctx->c_collision = Components_collision_create(count);

  for (size_t i = 0; i < count; i++) {
    // TRANSFORM
    if (i == 0) {
      // Top-left particle - moving diagonally down-right (center collision
      // group)
      ctx->c_transform->items[i].pos = (Vector2){.x = 80, .y = 80};
      ctx->c_transform->items[i].v =
          (Vector2){.x = 35.36, .y = 35.36}; // 50 at 45°
    } else if (i == 1) {
      // Bottom-right particle - moving diagonally up-left (center collision
      // group)
      ctx->c_transform->items[i].pos = (Vector2){.x = 220, .y = 220};
      ctx->c_transform->items[i].v =
          (Vector2){.x = -35.36, .y = -35.36}; // 50 at 225°
    } else if (i == 2) {
      // Top-right particle - moving diagonally down-left (center collision
      // group)
      ctx->c_transform->items[i].pos = (Vector2){.x = 220, .y = 80};
      ctx->c_transform->items[i].v =
          (Vector2){.x = -35.36, .y = 35.36}; // 50 at 135°
    } else if (i == 3) {
      // Bottom-left particle - moving diagonally up-right (center collision
      // group)
      ctx->c_transform->items[i].pos = (Vector2){.x = 80, .y = 220};
      ctx->c_transform->items[i].v =
          (Vector2){.x = 35.36, .y = -35.36}; // 50 at 315°
    } else if (i == 4) {
      // Left particle - moving right along x-axis (separate collision)
      ctx->c_transform->items[i].pos = (Vector2){.x = 300, .y = 100};
      ctx->c_transform->items[i].v = (Vector2){.x = 65, .y = 0};
    } else {
      // Top-right particle - moving diagonally down-left (separate collision)
      ctx->c_transform->items[i].pos = (Vector2){.x = 450, .y = 50};
      ctx->c_transform->items[i].v =
          (Vector2){.x = -35.36, .y = 35.36}; // 50 at 135°
    }
    ctx->c_transform->count++;

    // RENDER
    ctx->c_render->items[i].renderRadius = ctx->entitySize;
    ctx->c_render->items[i].color = (Color){255, 0, 0, 200};
    ctx->c_render->count++;

    // COLLISION
    ctx->c_collision->items[i].radius = ctx->entitySize;
    ctx->c_collision->items[i].mass = 5.0f;
    ctx->c_collision->count++;
  }
}
void init_collision_single_particle(SpatialContext *ctx) {
  size_t count = 1;
  ctx->entitiesCount = count;
  ctx->c_transform = Components_transform_create(count);
  ctx->c_render = Components_render_create(count);
  ctx->c_collision = Components_collision_create(count);
  for (size_t i = 0; i < count; i++) {
    // TRANSFORM
    if (i == 0) {
      // Top-left particle - moving diagonally down-right (center collision
      // group)
      ctx->c_transform->items[i].pos = (Vector2){.x = 80, .y = 80};
      ctx->c_transform->items[i].v =
          (Vector2){.x = 35.36, .y = 35.36}; // 50 at 45°

      ctx->c_transform->count++;

      // RENDER
      ctx->c_render->items[i].renderRadius = ctx->entitySize;
      ctx->c_render->items[i].color = (Color){255, 0, 0, 200};
      ctx->c_render->count++;

      // COLLISION
      ctx->c_collision->items[i].radius = ctx->entitySize;
      ctx->c_collision->items[i].mass = 5.0f;
      ctx->c_collision->count++;
    }
  }
}
void render_spatial_grid(SpatialGrid *sGrid) {

  // spatial grid - draw cells as rectangles
  // TODO: -1 on numx etc fixes the problem but why? might cause porlbmes
  for (size_t x = 0; x <= sGrid->numX - 1; x++) {
    for (size_t y = 0; y <= sGrid->numY - 1; y++) {
      float posX = x * sGrid->spacing;
      float posY = y * sGrid->spacing;
      Rectangle rec = (Rectangle){posX, posY, sGrid->spacing, sGrid->spacing};

      // Calculate cell index
      size_t idx = x * sGrid->numY + y;

      // Color based on whether the cell has entities
      Color cellColor;
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

      DrawRectangleRec(rec, (Color){10, 10, 10, 50});
      DrawRectangleLinesEx(rec, 1.0f, (Color){150, 150, 150, 255});
    }
  }

  // Flatten overlay
  Vector2 offset = {-100, -100};
  size_t size = 25.0f;
  for (size_t i = 0; i < sGrid->entities.capacity; i++) {
    Rectangle rec =
        (Rectangle){offset.x + i * (size + 5), offset.y, size, size};
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
    DrawRectangleLinesEx(rec, 2.0f, PINK);
    size_t val = sGrid->antitiesDense.items[i];
    char buf3[10];
    snprintf(buf3, 10, "%zu", val);
    DrawText(buf3, rec.x + (rec.width / 2), rec.y + (rec.height / 4), 14.0f,
             BLACK);
  }
}
void update_spatial(SpatialContext *ctx) {
  float spacing = ctx->sGrid.spacing;

  // Clear arrays
  memset(ctx->sGrid.entities.items, 0,
         ctx->sGrid.entities.capacity * sizeof(ctx->sGrid.entities.items[0]));
  ctx->sGrid.antitiesDense.count = 0;

  // First pass: count entities per cell
  for (size_t i = 0; i < ctx->entitiesCount; i++) {
    Component_transform *cT1 = &ctx->c_transform->items[i];

    float xi_f = cT1->pos.x / spacing;
    float yi_f = cT1->pos.y / spacing;
    if (xi_f < 0 || yi_f < 0)
      continue;

    size_t xi = (size_t)floorf(xi_f);
    size_t yi = (size_t)floorf(yi_f);

    if (xi >= ctx->sGrid.numX || yi >= ctx->sGrid.numY)
      continue;

    size_t idx = xi * ctx->sGrid.numY + yi;
    if (idx >= ctx->sGrid.entities.capacity)
      continue;

    // Count entities in this cell
    ctx->sGrid.entities.items[idx]++;
  }

  // Prefix sum to get start indices
  size_t sum = 0;
  for (size_t i = 0; i < ctx->sGrid.entities.capacity; i++) {
    size_t count = ctx->sGrid.entities.items[i];
    ctx->sGrid.entities.items[i] = sum; // Store start index
    sum += count;
  }

  // Resize dense array to fit all entities
  ctx->sGrid.antitiesDense.count = sum;

  // Second pass: fill dense array
  for (size_t i = 0; i < ctx->entitiesCount; i++) {
    Component_transform *cT1 = &ctx->c_transform->items[i];

    float xi_f = cT1->pos.x / spacing;
    float yi_f = cT1->pos.y / spacing;
    if (xi_f < 0 || yi_f < 0)
      continue;

    size_t xi = (size_t)floorf(xi_f);
    size_t yi = (size_t)floorf(yi_f);

    if (xi >= ctx->sGrid.numX || yi >= ctx->sGrid.numY)
      continue;

    size_t idx = xi * ctx->sGrid.numY + yi;
    if (idx >= ctx->sGrid.entities.capacity)
      continue;

    // Add entity to dense array at the next available position for this cell
    size_t denseIdx = ctx->sGrid.entities.items[idx];
    ctx->sGrid.antitiesDense.items[denseIdx] = i;
    ctx->sGrid.entities.items[idx]++; // Increment for next entity in this cell
  }

  // Restore start indices (optional, needed for querying)
  sum = 0;
  for (size_t i = 0; i < ctx->sGrid.entities.capacity; i++) {
    size_t end = ctx->sGrid.entities.items[i];
    ctx->sGrid.entities.items[i] = sum;
    sum = end;
  }
}
void sum_velocities(SpatialContext *ctx, Vector2 *out) {
  *out = (Vector2){0, 0}; // Initialize to zero

  for (size_t i = 0; i < ctx->entitiesCount; i++) {
    Component_transform *cTp1 = &ctx->c_transform->items[i];
    *out = Vector2Add(cTp1->v, *out);
  }
}
