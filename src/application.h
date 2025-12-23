#pragma once

#include "core/components.h"
#include "core/spatial.h"
#define _USE_MATH_DEFINES
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

} ApplicationContext;
void init_context(ApplicationContext *ctx);

// RENDER
void render(ApplicationContext *ctx);
void render_entities(Entities *ctx, Camera2D camera);
void render_spatial_grid(SpatialGrid *sGrid, Camera2D camera);
void render_info(ApplicationContext *ctx);
void render_entities_3D(Entities *e, Camera camera3D);
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
void update_entities_3D(Entities *ctx, float frameTime, float x_bound,
                        float y_bound, SpatialGrid *sGrid);

// COLLISION
void collision_simple_reverse(Entities *ctx, size_t idx1, size_t idx2);
void collision_elastic_separation(Entities *ctx, size_t idx1, size_t idx2);

void entities_update_collision(Entities *ctx, size_t idx, float frameTime,
                               float x_bound, float y_bound);
void particle_update_collision_spatial(Entities *ctx, size_t idx,
                                       float frameTime, float x_bound,
                                       float y_bound, SpatialGrid *sGrid);

// https://www.raylib.com/examples/models/loader.html?name=models_mesh_generation
static Mesh mesh_generate_circle(int segments) {
  Mesh mesh = {0};

  // 1 center vertex + segments edge vertices
  mesh.vertexCount = 1 + segments;
  mesh.triangleCount = segments;

  // Allocate vertex data
  mesh.vertices = (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
  mesh.texcoords = (float *)MemAlloc(mesh.vertexCount * 2 * sizeof(float));
  mesh.normals = (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
  mesh.indices = (unsigned short *)MemAlloc(mesh.triangleCount * 3 *
                                            sizeof(unsigned short));

  // Center vertex (index 0)
  mesh.vertices[0] = 0.0f;
  mesh.vertices[1] = 0.0f;
  mesh.vertices[2] = 0.0f;
  mesh.normals[0] = 0.0f;
  mesh.normals[1] = 0.0f;
  mesh.normals[2] = 1.0f;
  mesh.texcoords[0] = 0.5f;
  mesh.texcoords[1] = 0.5f;

  float theta = 2.0f * M_PI / segments;

  // Edge vertices
  for (int i = 0; i < segments; ++i) {
    float x = cos(i * theta);
    float y = sin(i * theta);

    int vertexIndex = (i + 1) * 3;
    mesh.vertices[vertexIndex + 0] = x;
    mesh.vertices[vertexIndex + 1] = y;
    mesh.vertices[vertexIndex + 2] = 0.0f;

    int normalIndex = (i + 1) * 3;
    mesh.normals[normalIndex + 0] = 0.0f;
    mesh.normals[normalIndex + 1] = 0.0f;
    mesh.normals[normalIndex + 2] = 1.0f;

    int texcoordIndex = (i + 1) * 2;
    mesh.texcoords[texcoordIndex + 0] = (x + 1.0f) * 0.5f;
    mesh.texcoords[texcoordIndex + 1] = (y + 1.0f) * 0.5f;
  }

  // Triangle indices (triangle fan from center)
  for (int i = 0; i < segments; ++i) {
    int triangleIndex = i * 3;
    mesh.indices[triangleIndex + 0] = 0;     // Center
    mesh.indices[triangleIndex + 1] = i + 1; // Current edge vertex
    mesh.indices[triangleIndex + 2] =
        ((i + 1) % segments) + 1; // Next edge vertex
  }

  // Upload mesh data to GPU
  UploadMesh(&mesh, false);

  return mesh;
}
