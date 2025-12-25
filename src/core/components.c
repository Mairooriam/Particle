#include "components.h"
#include "raymath.h"
#include <assert.h>
#include <stdio.h>
Entities *entities_create(size_t count) {
  Entities *e = malloc(sizeof(Entities));

  if (!e) {
    assert(0 && "LMAO get memory");
  }

  e->entitiesCount = count;
  e->c_collision = Components_collision_create(count);
  e->c_transform = Components_transform_create(count);
  e->c_render = Components_render_create(count);
  return e;
}
void entities_init(Entities *ctx, size_t count) {
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
}

void entities_free(Entities *ctx) {
  if (ctx->c_transform)
    Components_transform_free(ctx->c_transform);
  if (ctx->c_render)
    Components_render_free(ctx->c_render);
  if (ctx->c_collision)
    Components_collision_free(ctx->c_collision);
}
size_t entity_add_from_spec(Entities *ctx, EntitySpec spec) {
  size_t id = ctx->c_transform->count;

  ctx->c_transform->items[id] =
      (Component_transform){spec.pos, spec.vel, spec.acceleration, spec.restitution};
  ctx->c_transform->count++;

  ctx->c_render->items[id] =
      (Component_render){spec.color, spec.renderRadius, NULL};
  ctx->c_render->count++;

  ctx->c_collision->items[id] =
      (Component_collision){spec.collisionRadius, spec.mass,spec.inverseMass, 0, 0};
  ctx->c_collision->count++;

  return id;
}
void entity_init_collision_diagonal(Entities *ctx, SceneData data) {
  size_t count = 6;         // default
  float entitySize = 25.0f; // default
  float boundsX = 500.0f;   // default
  float boundsY = 500.0f;   // default

  if (data.is == ENTITY_INIT_DATAKIND_FULL) {
    count = data.get.full.count;
    entitySize = data.get.full.entitySize;
    boundsX = data.get.full.boundsX;
    boundsY = data.get.full.boundsY;
  } else if (data.is == ENTITY_INIT_DATAKIND_COUNT) {
    count = data.get.count;
  }

  entities_init(ctx, count);

  // Distribute entities in a grid pattern
  size_t cols = (size_t)sqrtf((float)count);
  size_t rows = (count + cols - 1) / cols;

  float x_spacing = boundsX / (float)(cols + 1);
  float y_spacing = boundsY / (float)(rows + 1);

  for (size_t i = 0; i < count; i++) {
    size_t col = i % cols;
    size_t row = i / cols;
    float thisEntitySize = entitySize * (0.8f + (i % 3) * 0.2f);

    // Vary velocities based on position
    float angle = (float)(i % 8) * 45.0f * DEG2RAD;
    float speed = 35.36f + (float)(i % 3) * 5.0f;

    float mass = 2.0f + ((float)rand() / RAND_MAX) * 50.0f;
    float inverseMass = (mass > 0.0f) ? 1.0f / mass : 0.0f;
    float restitution = 0.8f; 

    unsigned char r = (unsigned char)(255 * (1.0f - (mass - 2.0f) / 50.0f));
    unsigned char b = (unsigned char)(255 * ((mass - 2.0f) / 50.0f));
    Color color = (Color){r, 0, b, 200};

    EntitySpec spec = {
        .pos = {x_spacing * (float)(col + 1), y_spacing * (float)(row + 1), 0},
        .vel = {cosf(angle) * speed, sinf(angle) * speed, 0},
        .acceleration = {0, 9.81, 0},
        .renderRadius = thisEntitySize,
        .collisionRadius = thisEntitySize * 0.9f,
        .color = color,
        .mass = mass,
        .inverseMass = inverseMass,
        .restitution = restitution
    };

    entity_add_from_spec(ctx, spec);
  }
}

void sum_velocities(Entities *ctx, Vector3 *out) {
  *out = (Vector3){0, 0, 0}; // Initialize to zero

  for (size_t i = 0; i < ctx->entitiesCount; i++) {
    Component_transform *cTp1 = &ctx->c_transform->items[i];
    *out = Vector3Add(cTp1->v, *out);
  }
}

void log_velocities(Entities *ctx) {
  //  velocity increasing over simulation time !?
  Vector3 velocities;
  sum_velocities(ctx, &velocities);
  float lenght = Vector3Length(velocities);
  printf("(%f,%f) = %f\n", velocities.x, velocities.y, lenght);
}

void color_entities(Entities *ctx, Color color) {
  for (size_t i = 0; i < ctx->entitiesCount; i++) {
    Component_render *cRp1 = &ctx->c_render->items[i];
    cRp1->color = color;
  }
}

void update_entity_position(Component_transform *cTp1, float frameTime) {
  // v += a * dt
  cTp1->v = Vector3Add(cTp1->v, Vector3Scale(cTp1->a, frameTime));
  
  //pos += v * dt
  cTp1->pos = Vector3Add(cTp1->pos, Vector3Scale(cTp1->v, frameTime));
}
void update_entity_boundaries(Entities *ctx, size_t idx, float x_bound,
                              float x_bound_min, float y_bound,
                              float y_bound_min) {
  Component_transform *cTp1 = &ctx->c_transform->items[idx];

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
