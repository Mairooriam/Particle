#pragma once
#include "shared.h"
#include <stdint.h>

// ==================== COMPONENTS ====================
typedef struct {
  Color color;
  float renderRadius; // Visual size (can differ from collision radius)
} c_Render;

typedef struct {
  Vector3 pos;
  Vector3 v;           // Velocity
  Vector3 a;           // acceleration
  float restitution;   // 1 -> will bounce apart - 0 -> both will keep moving to
                       // same direction
  Vector3 previousPos; // https://gafferongames.com/post/fix_your_timestep/
} c_Transform;

typedef struct {
  float springConstat;
  float restLenght;
  size_t parent;
} c_Spring;

typedef struct {
  float radius;
  float mass;
  float inverseMass;
  size_t collisionCount;
  size_t searchCount;
  Vector3 forceAccum;
} c_Collision;

// ==================== ENTITY ====================
struct Entity {
  size_t id;
  uint64_t flags;
  c_Transform c_transform;
  c_Render c_render;
  c_Collision c_collision;
  struct Entity *spawnEntity;
  float spawnCount;
  float spawnRate;
  float clock;
  bool followMouse;
  c_Spring c_spring;
};
typedef struct Entity Entity;

// ==================== DYNAMIC ARRAYS ====================
typedef struct {
  Entity *items;
  size_t count;
  size_t capacity;
} arr_Entity;

typedef struct {
  size_t *items;
  size_t count;
  size_t capacity;
} arr_size_t;

typedef struct Entities {
  Entity *items;
  size_t count;
  size_t capacity;
} Entities;

// ==================== ENTITY POOL ====================
typedef struct {
  arr_size_t entities_sparse; // stores indicies for entities located in dense
  arr_Entity entities_dense;  // contigious entity arr
  size_t capacity; // Total capacity of pool. other arrays share the same size
  arr_size_t freeIds;
  size_t nextId; // Next unique ID
} EntityPool;

// ==================== ARENA ====================
typedef struct {
  uint8_t *base;
  size_t size;
  size_t used;
} memory_arena;

// ==================== SPATIAL ====================
typedef struct {
  int bX, bY; // Bounds x and y
  int spacing;
  size_t capacity;
  arr_size_t spatialDense;
  arr_size_t spatialSparse;
  size_t numY; // number of cells in Y
  size_t numX; // number of cells in X
  bool isInitalized;
} SpatialGrid;

SpatialGrid *spatialGrid_create(memory_arena *arena, size_t entityCount);
void spatialGrid_update_dimensions(SpatialGrid *sGrid, Vector3 minBounds,
                                   Vector3 maxBounds, int spacing);
void update_spatial(SpatialGrid *sGrid, arr_Entity *e);
