#pragma once

#include "core/components.h"
#include "core/spatial.h"
#include <stdint.h>

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

// CONTEXT
typedef Camera2D Camera2D;
typedef struct ApplicationContext {
  AppState state;
  EntitiesInitFn *InitFn;
  Entities *entities;
  SpatialGrid *sGrid;
  Camera2D camera;
  int x_bound, y_bound;
  float frameTime;
  bool paused;
  bool step_one_frame;

} ApplicationContext;
void init_context(ApplicationContext *ctx);

// RENDER
void render(ApplicationContext *ctx);
void render_entities(Entities *ctx);
void render_spatial_grid(SpatialGrid *sGrid);
void render_info(ApplicationContext *ctx);
// INPUT
void handle_input(ApplicationContext *ctx);

// UPDATE
void update(ApplicationContext *ctx);
void update_entities(Entities *ctx, float frameTime, float x_bound,
                     float y_bound, SpatialGrid *sGrid);

// COLLISION
void collision_simple_reverse(Entities *ctx, size_t idx1, size_t idx2);
void collision_elastic_separation(Entities *ctx, size_t idx1, size_t idx2);

void particle_update_collision(Entities *ctx, size_t idx, float frameTime,
                               float x_bound, float y_bound);
void particle_update_collision_spatial(Entities *ctx, size_t idx,
                                       float frameTime, float x_bound,
                                       float y_bound, SpatialGrid *sGrid);
