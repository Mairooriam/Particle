#pragma once
#include "shared.h"
#include "utils.h"
#include "application_types.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
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
void update_spawners(float frameTime, Entity *e, Entities *entities);

typedef struct {
  void *base;
  size_t size;
  size_t used;
} memory_arena;

memory_arena initialize_arena(void *buffer, size_t arena_size) {
  memory_arena arena = {0};
  arena.base = (char *)buffer;
  arena.size = arena_size;
  arena.used = 0;
  return arena;
}

void *arena_alloc(memory_arena *arena, size_t size) {
  size_t mask = 7; // aligned to 8-1 for mask
  size_t aligned_used = (arena->used + mask) & ~mask;
  Assert(aligned_used + size <= arena->size);
  if (aligned_used + size > arena->size) {
    return NULL;
  }
  void *ptr = arena->base + aligned_used;
  arena->used = aligned_used + size;
  return ptr;
}

int arena_free(memory_arena *arena) {
  arena->used = 0;
  memset(arena->base, 0, arena->size);
  return 0;
}
typedef struct {
  Input lastFrameInput;
  Vector2 mouseDelta;
  Entities entities;
  Mesh instancedMesh;
  Input inputLastFrame;
  bool instancedMeshUpdated;
  memory_arena permanentArena;
  memory_arena transientArena;
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

  mesh.vertices = (float *)arena_alloc(arena, 24 * 3 * sizeof(float));
  if (!mesh.vertices)
    return mesh; // Allocation failed
  memcpy(mesh.vertices, vertices, 24 * 3 * sizeof(float));

  mesh.texcoords = (float *)arena_alloc(arena, 24 * 2 * sizeof(float));
  if (!mesh.texcoords)
    return mesh;
  memcpy(mesh.texcoords, texcoords, 24 * 2 * sizeof(float));

  mesh.normals = (float *)arena_alloc(arena, 24 * 3 * sizeof(float));
  if (!mesh.normals)
    return mesh;
  memcpy(mesh.normals, normals, 24 * 3 * sizeof(float));

  mesh.indices =
      (unsigned short *)arena_alloc(arena, 36 * sizeof(unsigned short));
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
