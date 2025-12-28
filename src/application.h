#pragma once
#include "utils.h"
#include <stdbool.h>
#include <stdint.h>

// application.c
#define Assert(Expression)                                                     \
  if (!(Expression)) {                                                         \
    *(int *)0 = 0;                                                             \
  }
#define KiloBytes(value) ((value) * 1024)
#define MegaBytes(value) ((KiloBytes(value)) * 1024)
#define GigaBytes(value) ((MegaBytes(value)) * 1024)

typedef struct {
  float x, y;
} Position;

// GameState *init_gameState() {
//   GameState *result = malloc(sizeof(GameState));
//   if (!result) {
//     assert(0 && "LMAO failed");
//   }
//
//   result->pos.x = 0;
//   result->pos.y = 0;
//
//   return result;
// }

typedef struct {
  bool isInitialized;
  void *permamentMemory;
  size_t permanentMemorySize;
  void *transientMemory;
  size_t transientMemorySize;
} GameMemory;

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
#ifndef RAYLIB_H
typedef struct {
  unsigned char r, g, b, a;
} Color;

typedef struct {
  float x, y;
} Vector2;

typedef struct {
  float x, y, z;
} Vector3;

typedef struct {
  float x, y, z, w;
} Vector4;

typedef struct {
  float m0, m4, m8, m12;
  float m1, m5, m9, m13;
  float m2, m6, m10, m14;
  float m3, m7, m11, m15;
} Matrix;

typedef Vector4 Quaternion;

static inline Vector3 Vector3Add(Vector3 v1, Vector3 v2) {
  return (Vector3){v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
}

static inline Vector3 Vector3Scale(Vector3 v, float scale) {
  return (Vector3){v.x * scale, v.y * scale, v.z * scale};
}

static inline float Lerp(float start, float end, float amount) {
  return start + amount * (end - start);
}
#endif

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
DA_CREATE(Entities)
DA_FREE(Entities)
DA_INIT(Entities)
void entity_add(Entities *entities, Entity entity);
void update_entity_position(Entity *e, float frameTime,
                            Vector2 mouseWorldPosition);
void update_entity_boundaries(Entity *e, float x_bound, float x_bound_min,
                              float y_bound, float y_bound_min);
Entity entity_create_physics_particle(Vector3 pos, Vector3 velocity);
Entity entity_create_spawner_entity();

typedef enum {
  RENDER_RECTANGLE,
  RENDER_CIRCLE,
} RenderCommandType;

typedef struct {
  RenderCommandType type;
  union {
    struct {
      float x, y, width, height;
      Color color; // Updated
    } rectangle;
    struct {
      float centerX, centerY, radius;
      Color color; // Updated
    } circle;
  };
} RenderCommand;

#define MAX_RENDER_COMMANDS 1000000
typedef struct {
  RenderCommand commands[MAX_RENDER_COMMANDS];
  int count;
} RenderQueue;
typedef struct {
  Vector2 mousePos;  
  // Example expansions:
  // bool mouseButtons[3];  // Left, middle, right
  // bool keys[256];        // Keyboard state
} Input;
#define GAME_UPDATE(name) void name(GameMemory *gameMemory, Input* input, float frameTime)
typedef GAME_UPDATE(GameUpdate);
GAME_UPDATE(game_update_stub) {
  (void)gameMemory;
  (void)frameTime;
}

void push_render_command(RenderQueue *queue, RenderCommand cmd);

typedef struct {
  Position pos;
  Entities entities;
} GameState;

static inline void Entities_init_with_buffer(Entities *da, size_t cap, Entity *buffer) {
  da->count = 0;
  da->capacity = cap;
  da->items = buffer;
}
void update_spawners(float frameTime, Entity *e, Entities* entities);


