#include "components.h"
#include "raylib.h"
#include "raymath.h"
#include "utils.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

// void entity_init_collision_diagonal(Entities *ctx, SceneData data) {
//   size_t count = 6;         // default
//   float entitySize = 25.0f; // default
//   float boundsX = 500.0f;   // default
//   float boundsY = 500.0f;   // default
//
//   if (data.is == ENTITY_INIT_DATAKIND_FULL) {
//     count = data.get.full.count;
//     entitySize = data.get.full.entitySize;
//     boundsX = data.get.full.boundsX;
//     boundsY = data.get.full.boundsY;
//   } else if (data.is == ENTITY_INIT_DATAKIND_COUNT) {
//     count = data.get.count;
//   }
//
//   entities_init(ctx, count);
//
//   // Distribute entities in a grid pattern
//   size_t cols = (size_t)sqrtf((float)count);
//   size_t rows = (count + cols - 1) / cols;
//
//   float x_spacing = boundsX / (float)(cols + 1);
//   float y_spacing = boundsY / (float)(rows + 1);
//
//   for (size_t i = 0; i < count; i++) {
//     size_t col = i % cols;
//     size_t row = i / cols;
//     float thisEntitySize = entitySize * (0.8f + (i % 3) * 0.2f);
//
//     // Vary velocities based on position
//     float angle = (float)(i % 8) * 45.0f * DEG2RAD;
//     float speed = 35.36f + (float)(i % 3) * 5.0f;
//
//     float mass = 2.0f + ((float)rand() / RAND_MAX) * 50.0f;
//     float inverseMass = (mass > 0.0f) ? 1.0f / mass : 0.0f;
//     float restitution = 0.8f;
//
//     unsigned char r = (unsigned char)(255 * (1.0f - (mass - 2.0f) / 50.0f));
//     unsigned char b = (unsigned char)(255 * ((mass - 2.0f) / 50.0f));
//     Color color = (Color){r, 0, b, 200};
//
//     EntitySpec spec = {
//         .pos = {x_spacing * (float)(col + 1), y_spacing * (float)(row + 1),
//         0}, .vel = {cosf(angle) * speed, sinf(angle) * speed, 0},
//         .acceleration = {0, 9.81, 0},
//         .renderRadius = thisEntitySize,
//         .collisionRadius = thisEntitySize * 0.9f,
//         .color = color,
//         .mass = mass,
//         .inverseMass = inverseMass,
//         .restitution = restitution};
//
//     entity_add_from_spec(ctx, spec);
//   }
// }

// void sum_velocities(Entities *ctx, Vector3 *out) {
//   *out = (Vector3){0, 0, 0}; // Initialize to zero
//
//   for (size_t i = 0; i < ctx->entitiesCount; i++) {
//     Component_transform *cTp1 = &ctx->c_transform->items[i];
//     *out = Vector3Add(cTp1->v, *out);
//   }
// }

// void log_velocities(Entities *ctx) {
//   //  velocity increasing over simulation time !?
//   Vector3 velocities;
//   sum_velocities(ctx, &velocities);
//   float lenght = Vector3Length(velocities);
//   printf("(%f,%f) = %f\n", velocities.x, velocities.y, lenght);
// }

// void color_entities(Entities *ctx, Color color) {
//   for (size_t i = 0; i < ctx->entitiesCount; i++) {
//     Component_render *cRp1 = &ctx->c_render->items[i];
//     cRp1->color = color;
//   }
// }

void update_entity_position(Entity *e, float frameTime, Vector2 mouseWorldPos) {
  c_Transform *cTp1 = &e->c_transform;

  if (e->followMouse) {
    float lerpFactor = 0.1f;
    cTp1->pos.x = Lerp(cTp1->pos.x, mouseWorldPos.x, lerpFactor);
    cTp1->pos.y = Lerp(cTp1->pos.y, mouseWorldPos.y, lerpFactor);
    cTp1->pos.z = 0.0f;

    // TODO: disable physics while following to avoid interference maybe?!?
    cTp1->v = (Vector3){0, 0, 0};
    cTp1->a = (Vector3){0, 0, 0};
  } else {
    // Normal physics update
    cTp1->v = Vector3Add(cTp1->v, Vector3Scale(cTp1->a, frameTime));
    cTp1->pos = Vector3Add(cTp1->pos, Vector3Scale(cTp1->v, frameTime));
  }
}
void update_entity_boundaries(Entity *e, float x_bound, float x_bound_min,
                              float y_bound, float y_bound_min) {
  c_Transform *cTp1 = &e->c_transform;

  if (cTp1->pos.x < x_bound_min) {
    cTp1->pos.x = x_bound_min;
    cTp1->v.x = cTp1->v.x * -1;
  } else if (cTp1->pos.x > x_bound) {
    cTp1->pos.x = x_bound;
    cTp1->v.x = cTp1->v.x * -1;
  }

  if (cTp1->pos.y < y_bound_min) {
    cTp1->pos.y = y_bound_min;
    cTp1->v.y = cTp1->v.y * -1;
  } else if (cTp1->pos.y > y_bound) {
    cTp1->pos.y = y_bound;
    cTp1->v.y = cTp1->v.y * -1;
  }
}
void entity_add(Entities *entities, Entity entity) {
  if (!entities) {
    fprintf(stderr, "Entities pointer is NULL\n");
    return;
  }
  entity.id = entities->count;
  nob_da_append(entities, entity);
}
Entity entity_create_physics_particle(Vector3 pos, Vector3 velocity) {
  Entity e = {0};
  e.flags = ENTITY_FLAG_ACTIVE | ENTITY_FLAG_VISIBLE | ENTITY_FLAG_COLLIDING |
            ENTITY_FLAG_HAS_TRANSFORM | ENTITY_FLAG_HAS_RENDER |
            ENTITY_FLAG_HAS_COLLISION;
  e.c_transform = (c_Transform){.pos = {.x = pos.x, .y = pos.y, .z = 0},
                                .v = {.x = velocity.x, .y = velocity.y, .z = 0},
                                .a = {.x = 0, .y = 0, .z = 0},
                                .restitution = 0.90f};

  e.c_collision = (c_Collision){.radius = 25.0f,
                                .mass = 1.0f,
                                .inverseMass = 1.0f,
                                .collisionCount = 0,
                                .searchCount = 0};

  e.c_render = (c_Render){.renderRadius = 24.0f,
                          .color = {.r = 255, .b = 0, .g = 0, .a = 200}};

  // e.followMouse = true;
  return e;
}

Entity entity_create_spawner_entity() {
  Entity e = {0};
  e.flags = ENTITY_FLAG_HAS_SPAWNER;

  e.spawnRate = 20.0f;
  e.clock = 0;
  // e.followMouse = true;
  e.c_render = (c_Render){.renderRadius = 24.0f,
                          .color = {.r = 0, .b = 255, .g = 0, .a = 200}};
  e.spawnCount = 5;
  e.c_transform = (c_Transform){.pos = {.x = 8000, .y = 8000, .z = 0},
                                .v = {.x = -200, .y = -200, .z = 0},
                                .a = {.x = 0, .y = 0, .z = 0},
                                .restitution = 0.90f};
  e.spawnEntity = malloc(sizeof(Entity));

  *e.spawnEntity =
      entity_create_physics_particle((Vector3){0, 0, 0}, (Vector3){0, 0, 0});
  e.spawnEntity->c_transform.a = (Vector3){0, 9.81, 0};
  return e;
}
