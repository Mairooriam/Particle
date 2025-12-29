#pragma once
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
  char *base;
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
  Entities entities;
  memory_arena permanentArena;
  memory_arena transientArena;
} GameState;
