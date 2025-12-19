#pragma once

#include "raylib.h"
#include <stdint.h>
#include <stdlib.h>

#define TIME_IT(label, code) do { \
  double _start = GetTime(); \
  code; \
  printf("%s: %.3f ms\n", label, (GetTime() - _start) * 1000.0); \
} while(0)

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
  Vector2 pos;
  Vector2 v; // Velocity
} Particle;
void particle_update(SpatialContext *ctx, Particle *p);

typedef struct {
  int width, height;
} Window;

typedef struct {
  Particle *items;
  size_t count;
  size_t capacity;
} Particles;
DA_CREATE(Particles)
DA_FREE(Particles)
DA_INIT(Particles)

typedef Camera2D Camera2D;
typedef struct SpatialContext {
  Camera2D camera;
  AppState state;
  Animation animation;
  Particles *particles;
  Window window;
  float frameTime;
} SpatialContext;

void render(SpatialContext *ctx);
void handle_input(SpatialContext *ctx);
void handle_update(SpatialContext *ctx);
