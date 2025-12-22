#pragma once
#include "raylib.h" // just for types like Color & Vector2
#include "utils.h"  // For DA Macros

// RENDER COMPONENT
typedef struct {
  Color color;
  float renderRadius; // Visual size (can differ from collision radius)
  Model* model; // points to global model 
} Component_render;
typedef struct {
  Component_render *items;
  size_t count;
  size_t capacity;
} Components_render;
DA_CREATE(Components_render)
DA_FREE(Components_render)
DA_INIT(Components_render)

// TRANSFORM COMPONENT
typedef struct {
  Vector2 pos;
  Vector2 v; // Velocity
} Component_transform;
typedef struct {
  Component_transform *items;
  size_t count;
  size_t capacity;
} Components_transform;
DA_CREATE(Components_transform)
DA_FREE(Components_transform)
DA_INIT(Components_transform)

// COLLISION COMPONENT
typedef struct {
  float radius;
  float mass;
  size_t collisionCount;
  size_t searchCount;
} Component_collision;
typedef struct {
  Component_collision *items;
  size_t count;
  size_t capacity;
} Components_collision;
DA_CREATE(Components_collision)
DA_FREE(Components_collision)
DA_INIT(Components_collision)

typedef struct Entities {
  // Components
  size_t entitiesCount;
  Components_transform *c_transform;
  Components_render *c_render;
  Components_collision *c_collision;
} Entities; // is this even ecs? lazy ecs? each component arrau is same size as
            // entities count. not good

typedef enum {
  ENTITY_INIT_DATAKIND_NONE,
  ENTITY_INIT_DATAKIND_COUNT,
  ENTITY_INIT_DATAKIND_FULL,
} EntityInitDataKind;

typedef struct {
  EntityInitDataKind is;
  union Data {
    size_t count;
    struct {
      size_t count;
      float entitySize;
      float boundsX;
      float boundsY;
    } full;
  } get;
} SceneData;


typedef struct {
  Vector2 pos;
  Vector2 vel;
  float renderRadius;
  float collisionRadius;
  Color color;
  float mass;
} EntitySpec;


Entities *entities_create(size_t count);
void entities_init(Entities *ctx, size_t count);
void entities_free(Entities *ctx);
size_t entity_add_from_spec(Entities *ctx, EntitySpec spec);

// init stuff
void entity_init_collision_diagonal(Entities *ctx, SceneData data);

// Helpers
void sum_velocities(Entities *ctx, Vector2 *out);
void log_velocities(Entities *ctx);
void color_entities(Entities *ctx, Color color);

typedef void EntitiesInitFn(Entities *, SceneData);
