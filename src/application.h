#pragma once
#include "shared.h"
#include "utils.h"
#include "application_types.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
// application.c

static inline Vector3 Vector3Add(Vector3 v1, Vector3 v2) {
  return (Vector3){v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
}

static inline Vector2 Vector2Subtract(Vector2 v1, Vector2 v2) {
  return (Vector2){v1.x - v2.x, v1.y - v2.y};
}

static inline Vector3 Vector3Scale(Vector3 v, float scale) {
  return (Vector3){v.x * scale, v.y * scale, v.z * scale};
}

static inline float Lerp(float start, float end, float amount) {
  return start + amount * (end - start);
}
static Matrix MatrixTranslate(float x, float y, float z) {
  Matrix result = {1.0f, 0.0f, 0.0f, x, 0.0f, 1.0f, 0.0f, y,
                   0.0f, 0.0f, 1.0f, z, 0.0f, 0.0f, 0.0f, 1.0f};

  return result;
}
static inline Vector3 Vector3Lerp(Vector3 v1, Vector3 v2, float amount) {
  Vector3 result = {0};

  result.x = v1.x + amount * (v2.x - v1.x);
  result.y = v1.y + amount * (v2.y - v1.y);
  result.z = v1.z + amount * (v2.z - v1.z);

  return result;
}
// ================================
// ENTITY FLAGS (up to 64, using uint64_t)
// ================================

// Flag constants (bit positions 0-63)
#define ENTITY_FLAG_NONE 0ULL             // No flags set
#define ENTITY_FLAG_ACTIVE (1ULL << 0)    // Entity updates (e.g., physics)
#define ENTITY_FLAG_VISIBLE (1ULL << 1)   // Entity renders
#define ENTITY_FLAG_COLLIDING (1ULL << 2) // Entity participates in collisions
#define ENTITY_FLAG_DEAD (1ULL << 3)      // Entity is marked for removal

// Component flags (control if components are used)
#define ENTITY_FLAG_HAS_TRANSFORM                                              \
  (1ULL << 4) // Use c_transform (position/velocity)
#define ENTITY_FLAG_HAS_RENDER (1ULL << 5)    // Use c_render (visuals)
#define ENTITY_FLAG_HAS_COLLISION (1ULL << 6) // Use c_collision (physics)
#define ENTITY_FLAG_HAS_SPAWNER                                                \
  (1ULL << 7) // Use spawner fields (spawnRate, clock)

// Entity type flags (combine with core/component flags for functionality)
#define ENTITY_FLAG_PLAYER (1ULL << 8)      // Player-controlled
#define ENTITY_FLAG_ENEMY (1ULL << 9)       // AI enemy
#define ENTITY_FLAG_PROJECTILE (1ULL << 10) // Fast-moving projectile
#define ENTITY_FLAG_PARTICLE (1ULL << 11)   // Short-lived effect
#define ENTITY_FLAG_STATIC (1ULL << 12)     // Immovable object
#define ENTITY_FLAG_SPRING (1ULL << 13)
// ================================
// END ENTITY FLAGS
// ================================

void entity_add(Entities *entities, Entity entity);
void update_entity_position(Entity *e, float frameTime,
                            Vector2 mouseWorldPosition);
void update_entity_boundaries(Entity *e, float x_bound, float x_bound_min,
                              float y_bound, float y_bound_min, float z_bound,
                              float z_bound_min);
Entity entity_create_physics_particle(Vector3 pos, Vector3 velocity);
Entity entity_create_spawner_entity();

void push_render_command(RenderQueue *queue, RenderCommand cmd);

static inline void Entities_init_with_buffer(Entities *da, size_t cap,
                                             Entity *buffer) {
  da->count = 0;
  da->capacity = cap;
  da->items = buffer;
}

// TODO: make entities_dense be entities Dynamic array so it has count when
// passed around?

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

typedef struct {
  arr_size_t entities_sparse; // stores indicies for entities located in dense
  arr_Entity entities_dense;  // contigious entity arr
  size_t capacity; // Total capacity of pool. other arrays share the same size
  arr_size_t freeIds;
  size_t nextId; // Next unique ID
} EntityPool;
void update_spawners(float frameTime, Entity *e, EntityPool *entityPool);

typedef struct {
  uint8_t *base;
  size_t size;
  size_t used;
} memory_arena;

#define memory_index                                                           \
  size_t // from casey experiment and see if this is good way to do this

void arena_init(memory_arena *arena, uint8_t *base, memory_index arena_size) {
  arena->base = base;
  arena->size = arena_size;
  arena->used = 0;
}

#define arena_PushStruct(Arena, type) (type *)_PushStruct(Arena, sizeof(type))
#define arena_PushStructs(Arena, type, count)                                  \
  (type *)_PushStruct(Arena, sizeof(type) * count)
// Ment for internal use do not call
// Zeroes out the pushed struct
static void *_PushStruct(memory_arena *arena, size_t size) {
  Assert(arena->used + size <= arena->size);
  void *result = arena->base + arena->used;
  arena->used += size;
  return result;
}

// Initialize the entity pool
static EntityPool *EntityPoolInitInArena(memory_arena *arena, size_t capacity) {
  EntityPool *pool = arena_PushStruct(arena, EntityPool);
  pool->capacity = capacity;

  pool->entities_sparse.items = (size_t *)arena_PushStructs(
      arena, size_t, capacity + 1); // +1 for unused index 0
  pool->entities_sparse.capacity = capacity + 1;
  pool->entities_sparse.count = 0;

  pool->entities_dense.items =
      (Entity *)arena_PushStructs(arena, Entity, capacity);
  pool->entities_dense.capacity = capacity;
  pool->entities_dense.count = 0;

  pool->freeIds.items =
      (size_t *)arena_PushStructs(arena, size_t, capacity); // Free ID stack
  pool->freeIds.capacity = capacity;
  pool->freeIds.count = 0;

  pool->nextId = 1; // Start at 1

  for (size_t i = 0; i <= pool->entities_sparse.capacity;
       i++) { // Include index 0 (though not used)
    pool->entities_sparse.items[i] = SIZE_MAX;
  }

  return pool;
}
void PrintSparseAndDense(EntityPool *pool, size_t start, size_t end) {
  if (start >= pool->capacity) {
    printf("Error: Start index out of bounds.\n");
    return;
  }
  if (end > pool->capacity) {
    end = pool->capacity; // Clamp the end index to the capacity
  }

  printf("Sparse Array (from %zu to %zu):\n", start, end);
  for (size_t i = start; i < end; i++) {
    printf("Sparse[%zu] = %zu\n", i, pool->entities_sparse.items[i]);
  }

  printf("\nDense Array (from %zu to %zu):\n", start, end);
  for (size_t i = start; i < end && i < pool->entities_dense.count; i++) {
    printf("Dense[%zu] = { ID: %zu }\n", i, pool->entities_dense.items[i].id);
  }
  printf("\n");
}
static size_t EntityPoolGetNextId(EntityPool *pool) {
  if (pool->freeIds.count > 0) {
    return pool->freeIds.items[--pool->freeIds.count];
  } else {
    return pool->nextId++; // return new id
  }
}

static void EntityPoolPush(EntityPool *pool, Entity entity) {
  // If supplied entity ID == 0 -> choose id for it
  if (entity.id == 0) {
    entity.id = EntityPoolGetNextId(pool);
  } else {
    // If ID is pre-assigned, ensure it's free and valid
    // TODO: do i want to add flag to override exisiting? or do i want to delete
    // it first and then push new one?
    Assert(entity.id <= pool->capacity);
    Assert(pool->entities_sparse.items[entity.id] ==
           SIZE_MAX); // Ensure ID slot is free (prevent duplicates)
  }
  // Assert incase someone changes entity id to be not unsigned
  Assert(entity.id >= 0)

      // currently pool doesnt grow. assert entity is fits the pool
      Assert(entity.id <= pool->capacity);
  if (pool->entities_dense.count < pool->capacity) {
    // Add to the end of the dense array
    pool->entities_dense.items[pool->entities_dense.count] = entity;
    pool->entities_sparse.items[entity.id] =
        pool->entities_dense.count; // Use entity.id directly
    pool->entities_dense.count++;
  } else {
    printf("Error: Entity capacity reached.\n");
  }
}

static void EntityPoolRemoveIdx(EntityPool *pool, size_t idx) {
  // Safety check: Ensure the pool has entities and the ID is valid/active
  if (pool->entities_dense.count == 0) {
    // printf("Warning: Attempted to remove from an empty EntityPool.\n");
    return;
  }
  if (idx == 0 || idx > pool->capacity) {
    // printf("Warning: Invalid ID %zu for removal (out of range).\n", idx);
    return;
  }
  size_t denseIndex = pool->entities_sparse.items[idx]; // Use idx directly
  if (denseIndex == SIZE_MAX || denseIndex >= pool->entities_dense.count) {
    // printf("Warning: ID %zu is not active or invalid for removal.\n", idx);
    return;
  }

  // Swap with the last entity (even if it's the same)
  pool->entities_dense.items[denseIndex] =
      pool->entities_dense.items[pool->entities_dense.count - 1];
  size_t swappedId = pool->entities_dense.items[denseIndex].id;
  pool->entities_sparse.items[swappedId] = denseIndex; // Use swappedId directly

  // Mark removed ID as free
  pool->entities_sparse.items[idx] = SIZE_MAX; // Use idx directly
  pool->freeIds.items[pool->freeIds.count++] = idx;
  pool->entities_dense.count--;
}

typedef struct {
  Input lastFrameInput;
  Vector2 mouseDelta;
  Mesh instancedMesh;
  Input inputLastFrame;
  bool instancedMeshUpdated;
  memory_arena permanentArena;
  memory_arena transientArena;
  EntityPool *entityPool; // stores entities
  Vector3 minBounds;
  Vector3 maxBounds;
} GameState;
static Mesh GenMeshCube(memory_arena *arena, float width, float height,
                        float length) {
  Mesh mesh = {0};

  float vertices[] = {
      -width / 2,  -height / 2, length / 2,  width / 2,   -height / 2,
      length / 2,  width / 2,   height / 2,  length / 2,  -width / 2,
      height / 2,  length / 2,  -width / 2,  -height / 2, -length / 2,
      -width / 2,  height / 2,  -length / 2, width / 2,   height / 2,
      -length / 2, width / 2,   -height / 2, -length / 2, -width / 2,
      height / 2,  -length / 2, -width / 2,  height / 2,  length / 2,
      width / 2,   height / 2,  length / 2,  width / 2,   height / 2,
      -length / 2, -width / 2,  -height / 2, -length / 2, width / 2,
      -height / 2, -length / 2, width / 2,   -height / 2, length / 2,
      -width / 2,  -height / 2, length / 2,  width / 2,   -height / 2,
      -length / 2, width / 2,   height / 2,  -length / 2, width / 2,
      height / 2,  length / 2,  width / 2,   -height / 2, length / 2,
      -width / 2,  -height / 2, -length / 2, -width / 2,  -height / 2,
      length / 2,  -width / 2,  height / 2,  length / 2,  -width / 2,
      height / 2,  -length / 2};

  float texcoords[] = {
      0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
      0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
      1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
      0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f};

  float normals[] = {0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
                     1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  -1.0f, 0.0f,
                     0.0f,  -1.0f, 0.0f,  0.0f,  -1.0f, 0.0f,  0.0f,  -1.0f,
                     0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
                     0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  -1.0f, 0.0f,  0.0f,
                     -1.0f, 0.0f,  0.0f,  -1.0f, 0.0f,  0.0f,  -1.0f, 0.0f,
                     1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
                     0.0f,  1.0f,  0.0f,  0.0f,  -1.0f, 0.0f,  0.0f,  -1.0f,
                     0.0f,  0.0f,  -1.0f, 0.0f,  0.0f,  -1.0f, 0.0f,  0.0f};

  unsigned short indices[36];
  int k = 0;
  for (int i = 0; i < 36; i += 6) {
    indices[i] = 4 * k;
    indices[i + 1] = 4 * k + 1;
    indices[i + 2] = 4 * k + 2;
    indices[i + 3] = 4 * k;
    indices[i + 4] = 4 * k + 2;
    indices[i + 5] = 4 * k + 3;
    k++;
  }

  mesh.vertices = arena_PushStructs(arena, float, 24 * 3);
  if (!mesh.vertices)
    return mesh; // Allocation failed
  memcpy(mesh.vertices, vertices, 24 * 3 * sizeof(float));

  mesh.texcoords = arena_PushStructs(arena, float, 24 * 2);
  if (!mesh.texcoords)
    return mesh;
  memcpy(mesh.texcoords, texcoords, 24 * 2 * sizeof(float));

  mesh.normals = arena_PushStructs(arena, float, 24 * 3);
  if (!mesh.normals)
    return mesh;
  memcpy(mesh.normals, normals, 24 * 3 * sizeof(float));

  mesh.indices = arena_PushStructs(arena, unsigned short, 36);
  if (!mesh.indices)
    return mesh;
  memcpy(mesh.indices, indices, 36 * sizeof(unsigned short));

  mesh.vertexCount = 24;
  mesh.triangleCount = 12;

  return mesh;
}
static inline bool is_key_pressed(Input *current, Input *last, int key) {
  return current->keys[key] && !last->keys[key];
}

static inline bool is_key_released(Input *current, Input *last, int key) {
  return !current->keys[key] && last->keys[key];
}

static inline bool is_key_down(Input *current, int key) {
  return current->keys[key];
}

static inline bool is_key_up(Input *current, int key) {
  return !current->keys[key];
}

#define KEY_SPACE 32
#define KEY_ENTER 257
#define KEY_ESCAPE 256
#define KEY_BACKSPACE 259
#define KEY_TAB 258
#define KEY_LEFT_SHIFT 340
#define KEY_LEFT_CONTROL 341
#define KEY_LEFT_ALT 342
#define KEY_RIGHT_SHIFT 344
#define KEY_RIGHT_CONTROL 345
#define KEY_RIGHT_ALT 346
#define KEY_A 65
#define KEY_B 66
#define KEY_C 67
#define KEY_D 68
#define KEY_E 69
#define KEY_F 70
#define KEY_G 71
#define KEY_H 72
#define KEY_I 73
#define KEY_J 74
#define KEY_K 75
#define KEY_L 76
#define KEY_M 77
#define KEY_N 78
#define KEY_O 79
#define KEY_P 80
#define KEY_Q 81
#define KEY_R 82
#define KEY_S 83
#define KEY_T 84
#define KEY_U 85
#define KEY_V 86
#define KEY_W 87
#define KEY_X 88
#define KEY_Y 89
#define KEY_Z 90
#define KEY_0 48
#define KEY_1 49
#define KEY_2 50
#define KEY_3 51
#define KEY_4 52
#define KEY_5 53
#define KEY_6 54
#define KEY_7 55
#define KEY_8 56
#define KEY_9 57
#define KEY_LEFT 263
#define KEY_RIGHT 262
#define KEY_UP 265
#define KEY_DOWN 264
