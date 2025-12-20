#pragma once

#include "raylib.h"
#include <_mingw_off_t.h>
#include <stdint.h>
#include <stdlib.h>

#define TIME_IT(label, code)                                                   \
  do {                                                                         \
    double _start = GetTime();                                                 \
    code;                                                                      \
    printf("%s: %.3f ms\n", label, (GetTime() - _start) * 1000.0);             \
  } while (0)

#define DA_CREATE(type)                                                        \
  static inline type *type##_create(size_t cap) {                              \
    type *da = malloc(sizeof(type));                                           \
    if (!da)                                                                   \
      return NULL;                                                             \
    da->count = 0;                                                             \
    da->capacity = cap;                                                        \
    da->items = malloc(sizeof(*da->items) * cap);                              \
    if (!da->items) {                                                          \
      free(da);                                                                \
      return NULL;                                                             \
    }                                                                          \
    return da;                                                                 \
  }

#define DA_FREE(type)                                                          \
  static inline void type##_free(type *da) {                                   \
    if (da) {                                                                  \
      free(da->items);                                                         \
      free(da);                                                                \
    }                                                                          \
  }

#define DA_INIT(type)                                                          \
  static inline void type##_init(type *da, size_t cap) {                       \
    da->count = 0;                                                             \
    da->capacity = cap;                                                        \
    da->items = malloc(sizeof(*da->items) * cap);                              \
  }

typedef enum {
  APP_STATE_IDLE,
} AppState;

typedef enum {
  ANIMATION_IDLE,
} AnimationState;

typedef struct {
  float time_elapsed;
  float deltatime;
  float trig_time_current;
  float trig_target;
  size_t current_step;
  bool animation_playing;
  AnimationState state;
} Animation;

typedef struct {
  int hello;
} Grid;

typedef struct SpatialContext SpatialContext;
typedef struct {
  int width, height;
} Window;

// RENDER COMPONENT
typedef struct {
  Color color;
  float renderRadius; // Visual size (can differ from collision radius)
} Component_render;
typedef struct {
  Component_render *items;
  size_t count;
  size_t capacity;
} Components_render;
DA_CREATE(Components_render)
DA_FREE(Components_render)
DA_INIT(Components_render)

// TRANSFORM COMPONENT
typedef struct {
  Vector2 pos;
  Vector2 v; // Velocity
} Component_transform;
typedef struct {
  Component_transform *items;
  size_t count;
  size_t capacity;
} Components_transform;
DA_CREATE(Components_transform)
DA_FREE(Components_transform)
DA_INIT(Components_transform)

// COLLISION COMPONENT
typedef struct {
  float radius;
  float mass;
} Component_collision;
typedef struct {
  Component_collision *items;
  size_t count;
  size_t capacity;
} Components_collision;
DA_CREATE(Components_collision)
DA_FREE(Components_collision)
DA_INIT(Components_collision)

// CONTEXT
typedef Camera2D Camera2D;
typedef struct SpatialContext {
  Camera2D camera;
  AppState state;
  Animation animation;

  // Components
  Components_transform *c_transform;
  Components_render *c_render;
  Components_collision *c_collision;
  size_t entitiesCount;
  Window window;
  int x_bound, y_bound;
  float frameTime;
} SpatialContext;

void render(SpatialContext *ctx);
void handle_input(SpatialContext *ctx);
void handle_update(SpatialContext *ctx);
void particle_update_collision(SpatialContext *ctx, size_t idx);
void init(SpatialContext *ctx, size_t count);
