#pragma once

#include "ht.h"
#include "raylib.h"
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

// |------------------------------| ->> array of pointers size_t**
// [[0][1][2][3][4][5][6][7][8][9]]
//   ^
//   pointer to size_t*

typedef struct {
  size_t *items;
  size_t count;
  size_t capacity;
} arr_size_t;
DA_CREATE(arr_size_t)
DA_FREE(arr_size_t)
DA_INIT(arr_size_t)

typedef struct {
  arr_size_t *items;
  size_t count;
  size_t capacity;
} arr_size_t_ptr;
DA_CREATE(arr_size_t_ptr)
DA_FREE(arr_size_t_ptr)
DA_INIT(arr_size_t_ptr)

typedef struct {
  int bX, bY; // Bounds x and y
  int spacing;
  arr_size_t antitiesDense;
  arr_size_t entities;
  size_t numY; // number of cells in Y
  size_t numX; // number of cells in X
} SpatialGrid;

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
  size_t collisionCount;
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

  bool collisionHappened;
  size_t entitiesCount;
  Window window;
  int x_bound, y_bound;
  float frameTime;
  bool paused;
  bool step_one_frame;
  SpatialGrid sGrid;
  float entitySize;
} SpatialContext;

void render(SpatialContext *ctx);
void render_spatial_grid(SpatialGrid *sGrid);
void handle_input(SpatialContext *ctx);
void handle_update(SpatialContext *ctx);
void update_spatial(SpatialContext *ctx);

void collision_simple_reverse(SpatialContext *ctx, size_t idx1, size_t idx2);
void collision_elastic_separation(SpatialContext *ctx, size_t idx1,
                                  size_t idx2);

void particle_update_collision(SpatialContext *ctx, size_t idx);
void particle_update_collision_spatial(SpatialContext *ctx, size_t idx);

void init(SpatialContext *ctx, size_t count);
void init_collision_head_on(SpatialContext *ctx);
void init_collision_not_moving(SpatialContext *ctx);
void init_collision_diagonal(SpatialContext *ctx);
void init_collision_single_particle(SpatialContext *ctx);

void appLoopMain(SpatialContext *ctx);

void sum_velocities(SpatialContext *ctx, Vector2 *out);
