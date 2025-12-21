#include "spatial.h"
#include "raylib.h"
#include "raymath.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI  // Exclude GDI (prevents Rectangle conflict)
#define NOUSER // Exclude User32 (prevents CloseWindow, ShowCursor conflicts)
#include <windows.h>
#endif

#define NOB_IMPLEMENTATION
#include "nob.h"

void render(SpatialContext *ctx) {

  DrawRectangleLines(0, 0, ctx->y_bound, ctx->x_bound, BLACK);

  for (size_t i = 0; i < ctx->entitiesCount; i++) {
    Component_transform *cT = &ctx->c_transform->items[i];
    Component_render *cR = &ctx->c_render->items[i];

    DrawCircleSector(cT->pos, cR->renderRadius, 0, 360, 16, cR->color);

    // Vector2 n_vel = Vector2Normalize(cT->v);
    // DrawLine(cT->pos.x, cT->pos.y, cT->v.x, cT->v.y, BLACK);
    Vector2 v_relative = Vector2Add(cT->pos, cT->v);
    DrawLine(cT->pos.x, cT->pos.y, v_relative.x, v_relative.y, BLACK);
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
    init_collision_not_moving(ctx);
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
    particle_update_collision(ctx, i);
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
void init(SpatialContext *ctx, size_t count) {
  ctx->entitiesCount = count;

  // TODO: LEALLK
  ctx->c_transform = Components_transform_create(count);
  ctx->c_render = Components_render_create(count);
  ctx->c_collision = Components_collision_create(count);

  for (size_t i = 0; i < count; i++) {
    // TRANSFROM
    ctx->c_transform->items[i].pos =
        (Vector2){.x = (float)(i % ctx->window.width) * 12,
                  .y = (float)(i % ctx->window.height) * 12};
    ctx->c_transform->items[i].v =
        (Vector2){.x = (float)(i % 5) * 10.0f - 20.0f,
                  .y = (float)(i % 3) * 15.0f - 15.0f};
    ctx->c_transform->count++;

    // RENDER
    ctx->c_render->items[i].renderRadius = 5.0f;
    ctx->c_render->items[i].color = (Color){255, 0, 0, 200};
    ctx->c_render->count++;

    // COLLISION
    ctx->c_collision->items[i].radius = 5.0f;
    ctx->c_collision->items[i].mass = 5.0f;
    ctx->c_collision->count++;
  }
}

void init_collision_moving_to_not_moving(SpatialContext *ctx) {
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
    ctx->c_render->items[i].renderRadius = 5.0f;
    ctx->c_render->items[i].color = (Color){255, 0, 0, 200};
    ctx->c_render->count++;

    // collision
    ctx->c_collision->items[i].radius = 5.0f;
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
    ctx->c_render->items[i].renderRadius = 5.0f;
    ctx->c_render->items[i].color = (Color){255, 0, 0, 200};
    ctx->c_render->count++;

    // collision
    ctx->c_collision->items[i].radius = 5.0f;
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
