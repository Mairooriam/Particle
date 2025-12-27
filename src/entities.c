#include "entities.h"
#include "flecs.h"
ECS_COMPONENT_DECLARE(c_Transform);
ECS_COMPONENT_DECLARE(c_Render);
ECS_COMPONENT_DECLARE(c_Collision);
ECS_COMPONENT_DECLARE(c_Camera2D);
void register_components(ecs_world_t *world) {
  ECS_COMPONENT_DEFINE(world, c_Transform);
  ECS_COMPONENT_DEFINE(world, c_Render);
  ECS_COMPONENT_DEFINE(world, c_Collision);
  ECS_COMPONENT_DEFINE(world, c_Camera2D);
}
ecs_entity_t create_entity(ecs_world_t *world, c_Transform t, c_Collision c,
                           c_Render r) {

  ecs_entity_t entity = ecs_new(world);
  // Add components to the entity
  ecs_set(world, entity, c_Transform,
          {.pos = t.pos,
           .acceleration = t.acceleration,
           .restitution = t.restitution,
           .vel = t.vel});
  ecs_set(world, entity, c_Render,
          {.renderRadius = r.renderRadius, .color = r.color});
  ecs_set(world, entity, c_Collision,
          {.radius = c.radius, .inverseMass = c.inverseMass, .mass = c.mass});

  return entity;
}
