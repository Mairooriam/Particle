#pragma once

#include "internal/math/raymath.h"
#include "shared.h"
#include "application_types.h"
#include "entity_types.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "entityPool_types.h"
// application.c

typedef struct {
  Input lastFrameInput;
  Vector2 mouseDelta;
  // Mesh instancedMesh;
  Input inputLastFrame;
  bool instancedMeshUpdated;
  memory_arena permanentArena;
  memory_arena transientArena;
  MemoryAllocator permanentAllocator;
  MemoryAllocator transientAllocator;
  Vector3 minBounds;
  Vector3 maxBounds;

  EntityPool *entityPool; // stores entities
  SpatialGrid *sGrid;
} GameState;

// ==================== render ====================
void render(GameMemory *gameMemory, GameState *gameState);
void push_render_command(RenderQueue *queue, RenderCommand cmd);

// ==================== input ====================
void handle_input(GameState *gameState, Input *input);
void handle_update(GameState *gameState, float frameTime, Input *input);
void handle_init(GameMemory *gameMemory, GameState *gameState);

// ==================== update ====================
void update_entity_position(Entity *e, float frameTime,
                            Vector2 mouseWorldPosition);
void update_entity_boundaries(Entity *e, float x_bound, float x_bound_min,
                              float y_bound, float y_bound_min, float z_bound,
                              float z_bound_min);
void update_spawners(float frameTime, Entity *e, EntityPool *entityPool);
void update_collision(GameState *gameState, float frameTime);

Entity entity_create_physics_particle(Vector3 pos, Vector3 velocity);
Entity entity_create_spawner_entity(void);

// ==================== INPUT ====================
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

bool CheckCollisionCircles(Vector2 center1, float radius1, Vector2 center2,
                           float radius2) {
  bool collision = false;

  float dx = center2.x - center1.x; // X distance between centers
  float dy = center2.y - center1.y; // Y distance between centers

  float distanceSquared = dx * dx + dy * dy; // Distance between centers squared
  float radiusSum = radius1 + radius2;

  collision = (distanceSquared <= (radiusSum * radiusSum));

  return collision;
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
