#pragma once
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

extern ECS_COMPONENT_DECLARE(c_Transform);
extern ECS_COMPONENT_DECLARE(c_Render);
extern ECS_COMPONENT_DECLARE(c_Collision);
extern ECS_COMPONENT_DECLARE(c_Camera2D);



// FLECS examples/c/entities/fwd_declare_component
// Forward declaring a component will make the component id available from other
// functions, which fixes this error. "FLECS_IDComponentNameID" is undefined

// The forward declaration of the component id variable. This variable will have
// the name FLECS_IDPositionID, to ensure its name won't conflict with the type.

// When you want forward declare a component from a header, make sure to use the
// extern keyword to prevent multiple definitions of the same variable:
//
// In the header:
//   extern ECS_COMPONENT_DECLARE(Position);
//
// In *one* of the source files:
//  ECS_COMPONENT_DECLARE(Position);
