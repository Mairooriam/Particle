#include "application.h"
#include "core/components.h"
#include "core/spatial.h"
#include "core/utils.h"
#include "flecs.h"
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
void init_context(ApplicationContext *ctx, int bounds_x, int bounds_y) {
  ctx->world = ecs_init();
  ctx->state = APP_STATE_IDLE;
  Camera2D camera = {0};
  camera.zoom = 1.0f;
  ctx->camera = camera;
  ctx->y_bound = 500;
  ctx->x_bound = 500;
  ctx->paused = true;
  ctx->InitFn = entity_init_collision_diagonal;
  Camera camera3D = {
      {60.0f, 60.0f, 60.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 60.0f, 0};
  ctx->camera3D = camera3D;
  ctx->camera3D.projection = CAMERA_PERSPECTIVE;
  Entities *entities = entities_create();
  SpatialGrid *sGrid = SpatialGrid_create(bounds_x, bounds_y, 50, entities);

  ctx->entities = entities;
  ctx->sGrid = sGrid;
  ctx->x_bound = bounds_x;
  ctx->y_bound = bounds_y;
  Camera2D camera2d = {0};
  camera2d.zoom = 1.0f;
  ctx->camera = camera2d;
  ctx->state = APP_STATE_2D;
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
  if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
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
    camera->zoom = Clamp(expf(logf(camera->zoom) + scale), 0.01f, 64.0f);
  }
  static Vector2 initial = {0, 0};
  Vector2 current = GetMousePosition();
  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
    initial = GetMousePosition();
  }

  if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
    // Vector2 delta = Vector2Subtract(initial, current);
  }
  if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
    Vector2 tmpDelta = Vector2Subtract(current, initial);
    static Vector2 delta = {0}; // Keeps its value across function calls

    // Only update delta if the condition is met
    if (fabs(tmpDelta.x) > 10 && fabs(tmpDelta.y) > 10) {
      delta = tmpDelta; // Update delta
      printf("on release delta x:%f, y:%f\n", delta.x, delta.y);
    } else {
      printf("delta unchanged, using previous delta x:%f, y:%f\n", delta.x,
             delta.y);
    }

    EntitySpec spec = {.pos = (Vector3){initial.x, initial.y, 0},
                       .vel = (Vector3){delta.x, delta.y, 0},
                       .acceleration = {0, 9.81, 0},
                       .renderRadius = 25.0f,
                       .collisionRadius = 25.0f * 0.9f,
                       .color = GREEN,
                       .mass = 50.0f,
                       .inverseMass = 1 / 50.0f,
                       .restitution = 0.99};

    entity_add_from_spec(ctx->entities, spec);
    initial = (Vector2){0, 0};
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
  /* TIME_IT("Reset entity color", color_entities(ctx->entities, GREEN)); */

  TIME_IT("Update Entities",
          update_entities(ctx->entities, ctx->frameTime, ctx->x_bound,
                          ctx->y_bound, ctx->sGrid));

  TIME_IT("Update Spatial", update_spatial(ctx->sGrid, ctx->entities));
}

void update_entities(Entities *ctx, float frameTime, float x_bound,
                     float y_bound, SpatialGrid *sGrid) {

  // TRANSFORMS
  for (size_t i = 0; i < ctx->entitiesCount; i++) {
    update_entity_position(&ctx->c_transform->items[i], frameTime);
  }

  // MAP BOUNDARIES
  for (size_t i = 0; i < ctx->entitiesCount; i++) {
    update_entity_boundaries(ctx, i, x_bound, 0, y_bound, 0);
  }

  // COLLISIONS
  for (size_t i = 0; i < ctx->entitiesCount; i++) {
    particle_update_collision_spatial(ctx, i, sGrid);
  }
}

void update_entities_3D(Entities *ctx, float frameTime, SpatialGrid *sGrid,
                        Matrix *transforms) {
  (void)sGrid;
  for (size_t i = 0; i < ctx->entitiesCount; i++) {
    entities_update_collision_3D(ctx, i, frameTime);
  }
  // TODO: for now here move later
  for (size_t i = 0; i < ctx->entitiesCount; i++) {
    Component_transform *cTp1 = &ctx->c_transform->items[i];
    transforms[i] = MatrixTranslate(cTp1->pos.x, cTp1->pos.y, cTp1->pos.z);
  }
}
void collision_simple_reverse(Entities *ctx, size_t idx1, size_t idx2) {
  Component_transform *cTp1 = &ctx->c_transform->items[idx1];
  Component_transform *cTp2 = &ctx->c_transform->items[idx2];
  Component_render *cRp1 = &ctx->c_render->items[idx1];
  Component_render *cRp2 = &ctx->c_render->items[idx2];

  // Old implementation - just reverse
  cTp1->v = Vector3Scale(cTp1->v, -1);
  cTp2->v = Vector3Scale(cTp2->v, -1);
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
  Vector3 delta = Vector3Subtract(cTp1->pos, cTp2->pos);
  float distance = Vector3Length(delta);
  Vector3 normal = Vector3Normalize(delta);

  float overlap = (cCp1->radius + cCp2->radius) - distance;
  Vector3 separation = Vector3Scale(normal, overlap / 2.0f);
  cTp1->pos = Vector3Add(cTp1->pos, separation);
  cTp2->pos = Vector3Subtract(cTp2->pos, separation);

  // Swap velocities (for equal masses)
  Vector3 temp = cTp1->v;
  cTp1->v = cTp2->v;
  cTp2->v = temp;
  cTp1->v = Vector3Scale(cTp1->v, 0.9f);
  cTp2->v = Vector3Scale(cTp2->v, 0.9f);
  cRp1->color = (Color){0, 255, 0, 200};
  cRp2->color = (Color){0, 255, 0, 200};
}

void collision_separation_meow(Entities *ctx, size_t idx1, size_t idx2) {
  Component_transform *cTp1 = &ctx->c_transform->items[idx1];
  Component_transform *cTp2 = &ctx->c_transform->items[idx2];
  Component_collision *cCp1 = &ctx->c_collision->items[idx1];
  Component_collision *cCp2 = &ctx->c_collision->items[idx2];
  Component_render *cRp1 = &ctx->c_render->items[idx1];
  Component_render *cRp2 = &ctx->c_render->items[idx2];

  // New implementation - elastic with separation
  Vector3 delta = Vector3Subtract(cTp1->pos, cTp2->pos);
  float distance = Vector3Length(delta);
  Vector3 normal = Vector3Normalize(delta);

  float overlap = (cCp1->radius + cCp2->radius) - distance;
  Vector3 separation = Vector3Scale(normal, overlap / 2.0f);
  cTp1->pos = Vector3Add(cTp1->pos, separation);
  cTp2->pos = Vector3Subtract(cTp2->pos, separation);

  // Swap velocities (for equal masses)
  // add velocity to the normals direction
  Vector3 v1 = Vector3Multiply(cTp1->v, normal);
  Vector3 v2a = Vector3Scale(normal, -1.0f);
  Vector3 v2b = Vector3Multiply(v2a, cTp1->v);
  Vector3 temp = v1;
  cTp1->v = v2b;
  cTp2->v = temp;
  cTp1->v = Vector3Scale(cTp1->v, 0.9f);
  cTp2->v = Vector3Scale(cTp2->v, 0.9f);
  cRp1->color = (Color){0, 255, 0, 200};
  cRp2->color = (Color){0, 255, 0, 200};
}

void entities_update_collision_3D(Entities *ctx, size_t idx, float frameTime) {
  Component_transform *cTp1 = &ctx->c_transform->items[idx];

  Vector3 v1 = Vector3Scale(cTp1->v, frameTime);
  cTp1->pos = Vector3Add(cTp1->pos, v1);

  if (cTp1->pos.x < -50) {
    cTp1->pos.x = -50;
    cTp1->v.x = cTp1->v.x * -1;
  } else if (cTp1->pos.x > 50) {
    cTp1->pos.x = 50;
    cTp1->v.x = cTp1->v.x * -1;
  }

  if (cTp1->pos.y < -50) {
    cTp1->pos.y = -50;
    cTp1->v.y = cTp1->v.y * -1;
  } else if (cTp1->pos.y > 50) {
    cTp1->pos.y = 50;
    cTp1->v.y = cTp1->v.y * -1;
  }
}
void particle_update_collision_spatial(Entities *ctx, size_t idx,
                                       SpatialGrid *sGrid) {

  Component_transform *cTp1 = &ctx->c_transform->items[idx];
  // Component_render *cRp1 = &ctx->c_render->items[idx];
  Component_collision *cCp1 = &ctx->c_collision->items[idx];
  cCp1->searchCount = 0;
  cCp1->collisionCount = 0;

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
        // Component_render *cRp2 = &ctx->c_render->items[otherIdx];
        if (CheckCollisionCircles(
                (Vector2){cTp1->pos.x, cTp1->pos.y}, cCp1->radius,
                (Vector2){cTp2->pos.x, cTp2->pos.y}, cCp2->radius)) {
          Vector3 delta = Vector3Subtract(cTp1->pos, cTp2->pos);
          float distance = Vector3Length(delta);
          Vector3 n12 = Vector3Normalize(delta);

          float overlap = (cCp1->radius + cCp2->radius) - distance;
          if (overlap > 0) {
            Vector3 separation = Vector3Scale(n12, overlap / 2.0f);
            cTp1->pos = Vector3Add(cTp1->pos, separation);
            cTp2->pos = Vector3Subtract(cTp2->pos, separation);
          }
          cCp1->collisionCount++;
          Vector3 dV12 = Vector3Subtract(cTp1->v, cTp2->v);
          float Vs = Vector3DotProduct(dV12, n12); // separation velocity

          if (Vs > 0) {
            break; // contact is either separating or stationary no impulse
                   // needed
          }
          float nVs =
              -Vs *
              cTp1->restitution; // New separation velocity with restitution
          float deltaV = nVs - Vs;

          // TODO: not checking if no particle -> colliding with wall? etc? in
          // example there is

          float totalInverseMass = cCp1->inverseMass + cCp2->inverseMass;
          if (totalInverseMass <= 0)
            break; // infinite mass impulses have no effect

          float impulse = deltaV / totalInverseMass;

          Vector3 impulsePerIMass = Vector3Scale(n12, impulse);

          cTp1->v = Vector3Add(
              cTp1->v, Vector3Scale(impulsePerIMass, cCp1->inverseMass));
          cTp2->v = Vector3Subtract(
              cTp2->v, Vector3Scale(impulsePerIMass, cCp2->inverseMass));
        }
        /* int r = xi * 50 % 255; */
        /* int g = yi * 50 % 255; */
        /* cRp1->color = (Color){r, g, 0, 255}; */
        /* cRp2->color = (Color){r, g, 0, 100}; */
        cCp1->searchCount++;
      }
    }
  }
}

// ---------------------------------------------
// RENDERING
// ---------------------------------------------
void render(ApplicationContext *ctx) {

  DrawRectangleLines(0, 0, ctx->x_bound, ctx->y_bound, BLACK);

  // TIME_IT("Render Spatial Grid", render_spatial_grid(ctx->sGrid,
  // ctx->camera));
  TIME_IT("Render Entities", render_entities(ctx->entities, ctx->camera));
}

void render_entities(Entities *ctx, Camera2D camera) {
  for (size_t i = 0; i < ctx->entitiesCount; i++) {

    Component_transform *cT = &ctx->c_transform->items[i];

    Vector2 screen =
        GetWorldToScreen2D((Vector2){cT->pos.x, cT->pos.y}, camera);

    if (screen.x > 800 || screen.x < 0) {
      continue;
    }

    if (screen.y > 600 || screen.y < 0) {
      continue;
    }

    Component_render *cR = &ctx->c_render->items[i];
    Component_collision *cCp1 = &ctx->c_collision->items[i]; // TODO: temp
                                                             // debug;

    DrawCircleSector((Vector2){cT->pos.x, cT->pos.y}, cR->renderRadius, 0, 360,
                     16, cR->color);

    // Vector3 n_vel = Vector3Normalize(cT->v);
    // DrawLine(cT->pos.x, cT->pos.y, cT->v.x, cT->v.y, BLACK);
    Vector3 v_relative = Vector3Add((Vector3){cT->pos.x, cT->pos.y, 0}, cT->v);
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
void render_entities_3D(Mesh mesh, Material material, const Matrix *transforms,
                        size_t count) {
  DrawMeshInstanced(mesh, material, transforms, count);
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
    char buf[32];
    snprintf(buf, 32, "%zu", val);
    if (val != 0) {
      DrawRectangleLinesEx(rec, 2.0f, PINK);
    } else {
      DrawRectangleLinesEx(rec, 2.0f, BLACK);
    }
    DrawText(buf, rec.x + (rec.width / 2), rec.y + (rec.height / 4), 14.0f,
             BLACK);
    memset(buf, 0, sizeof(buf));
    snprintf(buf, 32, "%zu", i);
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
    char buf3[32];
    snprintf(buf3, 10, "%zu", val);
    DrawText(buf3, rec.x + (rec.width / 2), rec.y + (rec.height / 4), 14.0f,
             BLACK);
  }
}
// ---------------------------------------------
//
// ---------------------------------------------
void init_instanced_draw(Shader *shader, Matrix *transforms, Entities *entities,
                         Material *material) {
  // UPDATE transforms according to entities positions
  for (size_t i = 0; i < entities->entitiesCount; i++) {
    Component_transform *cTp1 = &entities->c_transform->items[i];
    transforms[i] = MatrixTranslate(cTp1->pos.x, cTp1->pos.y, cTp1->pos.z);
  }

  // SHADERS
  shader->locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(*shader, "mvp");
  shader->locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(*shader, "viewPos");
  int ambientLoc = GetShaderLocation(*shader, "ambient");
  SetShaderValue(*shader, ambientLoc, (float[4]){0.2f, 0.2f, 0.2f, 1.0f},
                 SHADER_UNIFORM_VEC4);

  // MATERIALS
  // TODO: look for proper way to handle shaders etc. is shader meant to be
  // passed by value?
  material->shader = *shader;
  material->maps[MATERIAL_MAP_DIFFUSE].color = RED;
}

Mesh mesh_generate_circle(int segments) {
  Mesh mesh = {0};

  // 1 center vertex + segments edge vertices
  mesh.vertexCount = 1 + segments;
  mesh.triangleCount = segments;

  // Allocate vertex data
  mesh.vertices = (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
  mesh.texcoords = (float *)MemAlloc(mesh.vertexCount * 2 * sizeof(float));
  mesh.normals = (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
  mesh.indices = (unsigned short *)MemAlloc(mesh.triangleCount * 3 *
                                            sizeof(unsigned short));

  // Center vertex (index 0)
  mesh.vertices[0] = 0.0f;
  mesh.vertices[1] = 0.0f;
  mesh.vertices[2] = 0.0f;
  mesh.normals[0] = 0.0f;
  mesh.normals[1] = 0.0f;
  mesh.normals[2] = 1.0f;
  mesh.texcoords[0] = 0.5f;
  mesh.texcoords[1] = 0.5f;

  float theta = 2.0f * M_PI / segments;

  // Edge vertices
  for (int i = 0; i < segments; ++i) {
    float x = cos(i * theta);
    float y = sin(i * theta);

    int vertexIndex = (i + 1) * 3;
    mesh.vertices[vertexIndex + 0] = x;
    mesh.vertices[vertexIndex + 1] = y;
    mesh.vertices[vertexIndex + 2] = 0.0f;

    int normalIndex = (i + 1) * 3;
    mesh.normals[normalIndex + 0] = 0.0f;
    mesh.normals[normalIndex + 1] = 0.0f;
    mesh.normals[normalIndex + 2] = 1.0f;

    int texcoordIndex = (i + 1) * 2;
    mesh.texcoords[texcoordIndex + 0] = (x + 1.0f) * 0.5f;
    mesh.texcoords[texcoordIndex + 1] = (y + 1.0f) * 0.5f;
  }

  // Triangle indices (triangle fan from center)
  for (int i = 0; i < segments; ++i) {
    int triangleIndex = i * 3;
    mesh.indices[triangleIndex + 0] = 0;     // Center
    mesh.indices[triangleIndex + 1] = i + 1; // Current edge vertex
    mesh.indices[triangleIndex + 2] =
        ((i + 1) % segments) + 1; // Next edge vertex
  }

  // Upload mesh data to GPU
  UploadMesh(&mesh, false);

  return mesh;
}
