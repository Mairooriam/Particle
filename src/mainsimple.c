#include "raymath.h"
#include <flecs.h>
#include <raylib.h>
#include <stdio.h>

typedef struct Position {
  Vector3 pos;
} Position;
int main(void) {
  ecs_world_t *world = ecs_init();

  // Register the component
  ECS_COMPONENT(world, Position);

  // Create an entity and set the component using a compound literal
  ecs_entity_t e = ecs_new(world);
  Position pos = {.pos = {10, 10, 0}};
  ecs_set(world, e, Position, {.pos = pos.pos}); // Position pos = {.pos.x = 10,
                                                 // .pos.y = 10, pos.pos.z = 0};
  // ecs_set(world, e, Position,
  //         {.pos.x = pos.pos.x, .pos.y = pos.pos.y, .pos.z = pos.pos.z});

  // Retrieve and print the component
  const Position *p = ecs_get(world, e, Position);
  printf("Entity %llu Position: %f, %f, %f\n", e, p->pos.x, p->pos.y, p->pos.z);

  ecs_fini(world);
  return 0;
}
