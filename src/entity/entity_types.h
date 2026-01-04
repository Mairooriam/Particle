#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

typedef struct {
  float x, y;
} Vector2;

typedef struct {
  float x, y, z;
} Vector3;

typedef struct {
  Vector3 pos;
  Vector3 v;           // Velocity
  Vector3 a;           // acceleration
  float restitution;   // 1 -> will bounce apart - 0 -> both will keep moving to
                       // same direction
  Vector3 previousPos; // https://gafferongames.com/post/fix_your_timestep/
                       // currently not being used
  Vector3 lastGridPos; // last position used for spatial grid
  bool needsGridUpdate; // flag if entity moved enough to need update
} c_Transform;

typedef struct Entity {
  uint64_t identifier;
  Vector2 position;
  c_Transform transform;
} Entity;

typedef struct {
  Entity *items;
  size_t count;
  size_t capacity;
} arr_Entity;
