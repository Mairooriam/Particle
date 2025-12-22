#pragma once
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
  
  ctx->c_transform->items[id] = (Component_transform){spec.pos, spec.vel};
  ctx->c_transform->count++;
  
  ctx->c_render->items[id] = (Component_render){spec.color, spec.renderRadius};
  ctx->c_render->count++;
  
  ctx->c_collision->items[id] = (Component_collision){spec.collisionRadius, spec.mass, 0, 0};
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

    EntitySpec spec = {
      .pos = {x_spacing * (float)(col + 1), y_spacing * (float)(row + 1)},
      .vel = {cosf(angle) * speed, sinf(angle) * speed},
      .renderRadius = thisEntitySize,
      .collisionRadius = thisEntitySize * 0.9f,
      .color = (Color){255, 0, 0, 200},
      .mass = 5.0f
    };
    
    entity_add_from_spec(ctx, spec);
  }
}

void sum_velocities(Entities *ctx, Vector2 *out) {
  *out = (Vector2){0, 0}; // Initialize to zero

  for (size_t i = 0; i < ctx->entitiesCount; i++) {
    Component_transform *cTp1 = &ctx->c_transform->items[i];
    *out = Vector2Add(cTp1->v, *out);
  }
}

void log_velocities(Entities *ctx) {
  //  velocity increasing over simulation time !?
  Vector2 velocities;
  sum_velocities(ctx, &velocities);
  float lenght = Vector2Length(velocities);
  printf("(%f,%f) = %f\n", velocities.x, velocities.y, lenght);
}

void color_entities(Entities *ctx, Color color) {
  for (size_t i = 0; i < ctx->entitiesCount; i++) {
    Component_render *cRp1 = &ctx->c_render->items[i];
    cRp1->color = color;
  }
}
