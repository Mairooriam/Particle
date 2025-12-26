#pragma once

#include "core/components.h"
#include "core/spatial.h"
#define _USE_MATH_DEFINES
#include "flecs.h"
#include <math.h>
#include <stdint.h>
typedef enum { APP_STATE_IDLE, APP_STATE_3D, APP_STATE_2D } AppState;

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
  Camera camera3D;
  int x_bound, y_bound;
  float frameTime;
  bool paused;
  bool step_one_frame;
  ecs_world_t *world;
} ApplicationContext;
void init_context(ApplicationContext *ctx, int bounds_x, int bounds_y);

// RENDER
void render(ApplicationContext *ctx);
void render_entities(Entities *ctx, Camera2D camera);
void render_spatial_grid(SpatialGrid *sGrid, Camera2D camera);
void render_info(ApplicationContext *ctx);
void render_entities_3D(Mesh mesh, Material material, const Matrix *transforms,
                        size_t count);
// INPUT
void handle_input(ApplicationContext *ctx);
void input(ApplicationContext *ctx);
void input_state(ApplicationContext *ctx);
void input_mouse_2D(ApplicationContext *ctx);
void input_mouse_3D(ApplicationContext *ctx);
void input_other(ApplicationContext *ctx);
// UPDATE
void update(ApplicationContext *ctx);
void update_entities(Entities *ctx, float frameTime, float x_bound,
                     float y_bound, SpatialGrid *sGrid);
void update_entities_3D(Entities *ctx, float frameTime, SpatialGrid *sGrid,
                        Matrix *transforms);

// COLLISION
void collision_simple_reverse(Entities *ctx, size_t idx1, size_t idx2);
void collision_elastic_separation(Entities *ctx, size_t idx1, size_t idx2);
void collision_separation_meow(Entities *ctx, size_t idx1, size_t idx2);

void entities_update_collision_3D(Entities *ctx, size_t idx, float frameTime);
void particle_update_collision_spatial(Entities *ctx, size_t idx,
                                       SpatialGrid *sGrid);

void init_instanced_draw(Shader *shader, Matrix *transforms, Entities *entities,
                         Material *material);

// https://www.raylib.com/examples/models/loader.html?name=models_mesh_generation
Mesh mesh_generate_circle(int segments);
