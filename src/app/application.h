#pragma once
#include "shared/internal/memory_allocator.h"
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
// void push_render_command(RenderQueue *queue, RenderCommand cmd);

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

#include <SDL3/SDL_scancode.h>

#define KEY_SPACE        SDL_SCANCODE_SPACE
#define KEY_ENTER        SDL_SCANCODE_RETURN
#define KEY_ESCAPE       SDL_SCANCODE_ESCAPE
#define KEY_BACKSPACE    SDL_SCANCODE_BACKSPACE
#define KEY_TAB          SDL_SCANCODE_TAB
#define KEY_LEFT_SHIFT   SDL_SCANCODE_LSHIFT
#define KEY_LEFT_CONTROL SDL_SCANCODE_LCTRL
#define KEY_LEFT_ALT     SDL_SCANCODE_LALT
#define KEY_RIGHT_SHIFT  SDL_SCANCODE_RSHIFT
#define KEY_RIGHT_CONTROL SDL_SCANCODE_RCTRL
#define KEY_RIGHT_ALT    SDL_SCANCODE_RALT
#define KEY_A  SDL_SCANCODE_A
#define KEY_B  SDL_SCANCODE_B
#define KEY_C  SDL_SCANCODE_C
#define KEY_D  SDL_SCANCODE_D
#define KEY_E  SDL_SCANCODE_E
#define KEY_F  SDL_SCANCODE_F
#define KEY_G  SDL_SCANCODE_G
#define KEY_H  SDL_SCANCODE_H
#define KEY_I  SDL_SCANCODE_I
#define KEY_J  SDL_SCANCODE_J
#define KEY_K  SDL_SCANCODE_K
#define KEY_L  SDL_SCANCODE_L
#define KEY_M  SDL_SCANCODE_M
#define KEY_N  SDL_SCANCODE_N
#define KEY_O  SDL_SCANCODE_O
#define KEY_P  SDL_SCANCODE_P
#define KEY_Q  SDL_SCANCODE_Q
#define KEY_R  SDL_SCANCODE_R
#define KEY_S  SDL_SCANCODE_S
#define KEY_T  SDL_SCANCODE_T
#define KEY_U  SDL_SCANCODE_U
#define KEY_V  SDL_SCANCODE_V
#define KEY_W  SDL_SCANCODE_W
#define KEY_X  SDL_SCANCODE_X
#define KEY_Y  SDL_SCANCODE_Y
#define KEY_Z  SDL_SCANCODE_Z
#define KEY_0  SDL_SCANCODE_0
#define KEY_1  SDL_SCANCODE_1
#define KEY_2  SDL_SCANCODE_2
#define KEY_3  SDL_SCANCODE_3
#define KEY_4  SDL_SCANCODE_4
#define KEY_5  SDL_SCANCODE_5
#define KEY_6  SDL_SCANCODE_6
#define KEY_7  SDL_SCANCODE_7
#define KEY_8  SDL_SCANCODE_8
#define KEY_9  SDL_SCANCODE_9
#define KEY_LEFT  SDL_SCANCODE_LEFT
#define KEY_RIGHT SDL_SCANCODE_RIGHT
#define KEY_UP    SDL_SCANCODE_UP
#define KEY_DOWN  SDL_SCANCODE_DOWN