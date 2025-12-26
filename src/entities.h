#ifndef ENTITIES_H
#define ENTITIES_H
#include "raylib.h"

#include "flecs.h"

typedef struct {
  Vector3 pos;
  Vector3 vel;
  Vector3 acceleration;
  float restitution;
} c_Transform;

typedef struct {
  float renderRadius;
  Color color;
} c_Render;

typedef struct {
  float radius;
  float mass;
  float inverseMass;
} c_Collision;

typedef struct {
  Camera2D camera;
} c_Camera2D;

void register_components(ecs_world_t *world);

ecs_entity_t create_entity(ecs_world_t *world, c_Transform transform,
                           c_Collision collision, c_Render render);

// ECS_COMPONENT_DECLARE(c_Transform);
// ECS_COMPONENT_DECLARE(c_Render);
// ECS_COMPONENT_DECLARE(c_Collision);
// ECS_COMPONENT_DECLARE(c_Camera2D);
#endif // ENTITIES_H
