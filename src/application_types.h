#pragma once
#include "shared.h"
#include <stdint.h>

// ----------------------------
// ENTITIES
// ----------------------------

// RENDER COMPONENT
typedef struct {
  Color color;
  float renderRadius; // Visual size (can differ from collision radius)
} c_Render;

// TRANSFORM COMPONENT
typedef struct {
  Vector3 pos;
  Vector3 v;         // Velocity
  Vector3 a;         // acceleration
  float restitution; // 1 -> will bounce apart - 0 -> both will keep moving to
                     // same direction
  Vector3 previousPos; // https://gafferongames.com/post/fix_your_timestep/
} c_Transform;

typedef struct {
  float springConstat;
  float restLenght;
  size_t parent;
} c_Spring;

// COLLISION COMPONENT
typedef struct {
  float radius;
  float mass;
  float inverseMass;
  size_t collisionCount;
  size_t searchCount;
  Vector3 forceAccum;
} c_Collision;
typedef struct Entity Entity;
typedef struct Entity {
  size_t id;
  uint64_t flags;
  c_Transform c_transform;
  c_Render c_render;
  c_Collision c_collision;
  Entity *spawnEntity;
  float spawnCount;
  float spawnRate;
  float clock;
  bool followMouse;
  c_Spring c_spring;
} Entity;
typedef struct Entities {
  Entity *items;
  size_t count;
  size_t capacity;
} Entities;
