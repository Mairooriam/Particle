#pragma once
#include "entityPool_types.h"
#include "shared.h"
#include "application_types.h"
#include "entity_types.h"
#include "memory_allocator.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "entityPool.h"
// application.c

static inline Vector2 Vector2Subtract(Vector2 v1, Vector2 v2) {
  return (Vector2){v1.x - v2.x, v1.y - v2.y};
}

static inline float Lerp(float start, float end, float amount) {
  return start + amount * (end - start);
}
static Matrix MatrixTranslate(float x, float y, float z) {
  Matrix result = {1.0f, 0.0f, 0.0f, x, 0.0f, 1.0f, 0.0f, y,
                   0.0f, 0.0f, 1.0f, z, 0.0f, 0.0f, 0.0f, 1.0f};

  return result;
}
static inline Vector3 Vector3Lerp(Vector3 v1, Vector3 v2, float amount) {
  Vector3 result = {0};

  result.x = v1.x + amount * (v2.x - v1.x);
  result.y = v1.y + amount * (v2.y - v1.y);
  result.z = v1.z + amount * (v2.z - v1.z);

  return result;
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
Vector3 Vector3Zero(void) {
  Vector3 result = {0.0f, 0.0f, 0.0f};

  return result;
}

// Vector with components value 1.0f
Vector3 Vector3One(void) {
  Vector3 result = {1.0f, 1.0f, 1.0f};

  return result;
}

// Add two vectors
Vector3 Vector3Add(Vector3 v1, Vector3 v2) {
  Vector3 result = {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};

  return result;
}

// Add vector and float value
Vector3 Vector3AddValue(Vector3 v, float add) {
  Vector3 result = {v.x + add, v.y + add, v.z + add};

  return result;
}

// Subtract two vectors
Vector3 Vector3Subtract(Vector3 v1, Vector3 v2) {
  Vector3 result = {v1.x - v2.x, v1.y - v2.y, v1.z - v2.z};

  return result;
}

// Subtract vector by float value
Vector3 Vector3SubtractValue(Vector3 v, float sub) {
  Vector3 result = {v.x - sub, v.y - sub, v.z - sub};

  return result;
}

// Multiply vector by scalar
Vector3 Vector3Scale(Vector3 v, float scalar) {
  Vector3 result = {v.x * scalar, v.y * scalar, v.z * scalar};

  return result;
}

// Multiply vector by vector
Vector3 Vector3Multiply(Vector3 v1, Vector3 v2) {
  Vector3 result = {v1.x * v2.x, v1.y * v2.y, v1.z * v2.z};

  return result;
}

// Calculate two vectors cross product
Vector3 Vector3CrossProduct(Vector3 v1, Vector3 v2) {
  Vector3 result = {v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z,
                    v1.x * v2.y - v1.y * v2.x};

  return result;
}

// Calculate one vector perpendicular vector
Vector3 Vector3Perpendicular(Vector3 v) {
  Vector3 result = {0};

  float min = fabsf(v.x);
  Vector3 cardinalAxis = {1.0f, 0.0f, 0.0f};

  if (fabsf(v.y) < min) {
    min = fabsf(v.y);
    Vector3 tmp = {0.0f, 1.0f, 0.0f};
    cardinalAxis = tmp;
  }

  if (fabsf(v.z) < min) {
    Vector3 tmp = {0.0f, 0.0f, 1.0f};
    cardinalAxis = tmp;
  }

  // Cross product between vectors
  result.x = v.y * cardinalAxis.z - v.z * cardinalAxis.y;
  result.y = v.z * cardinalAxis.x - v.x * cardinalAxis.z;
  result.z = v.x * cardinalAxis.y - v.y * cardinalAxis.x;

  return result;
}

// Calculate vector length
float Vector3Length(const Vector3 v) {
  float result = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);

  return result;
}

// Calculate vector square length
float Vector3LengthSqr(const Vector3 v) {
  float result = v.x * v.x + v.y * v.y + v.z * v.z;

  return result;
}

// Calculate two vectors dot product
float Vector3DotProduct(Vector3 v1, Vector3 v2) {
  float result = (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z);

  return result;
}

// Calculate distance between two vectors
float Vector3Distance(Vector3 v1, Vector3 v2) {
  float result = 0.0f;

  float dx = v2.x - v1.x;
  float dy = v2.y - v1.y;
  float dz = v2.z - v1.z;
  result = sqrtf(dx * dx + dy * dy + dz * dz);

  return result;
}

// Calculate square distance between two vectors
float Vector3DistanceSqr(Vector3 v1, Vector3 v2) {
  float result = 0.0f;

  float dx = v2.x - v1.x;
  float dy = v2.y - v1.y;
  float dz = v2.z - v1.z;
  result = dx * dx + dy * dy + dz * dz;

  return result;
}
// Negate provided vector (invert direction)
Vector3 Vector3Negate(Vector3 v) {
  Vector3 result = {-v.x, -v.y, -v.z};

  return result;
}

// Divide vector by vector
Vector3 Vector3Divide(Vector3 v1, Vector3 v2) {
  Vector3 result = {v1.x / v2.x, v1.y / v2.y, v1.z / v2.z};

  return result;
}

// Normalize provided vector
Vector3 Vector3Normalize(Vector3 v) {
  Vector3 result = v;

  float length = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
  if (length != 0.0f) {
    float ilength = 1.0f / length;

    result.x *= ilength;
    result.y *= ilength;
    result.z *= ilength;
  }

  return result;
}
void update_entity_position(Entity *e, float frameTime,
                            Vector2 mouseWorldPosition);
void update_entity_boundaries(Entity *e, float x_bound, float x_bound_min,
                              float y_bound, float y_bound_min, float z_bound,
                              float z_bound_min);
Entity entity_create_physics_particle(Vector3 pos, Vector3 velocity);
Entity entity_create_spawner_entity();

void push_render_command(RenderQueue *queue, RenderCommand cmd);

// static inline void Entities_init_with_buffer(Entities *da, size_t cap,
//                                              Entity *buffer) {
//   da->count = 0;
//   da->capacity = cap;
//   da->items = buffer;
// }

// TODO: make entities_dense be entities Dynamic array so it has count when
// passed around?

void update_spawners(float frameTime, Entity *e, EntityPool *entityPool);

typedef struct {
  Input lastFrameInput;
  Vector2 mouseDelta;
  Mesh instancedMesh;
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
static Mesh GenMeshCube(MemoryAllocator *allocator, float width, float height,
                        float length) {
  Mesh mesh = {0};

  float vertices[] = {
      -width / 2,  -height / 2, length / 2,  width / 2,   -height / 2,
      length / 2,  width / 2,   height / 2,  length / 2,  -width / 2,
      height / 2,  length / 2,  -width / 2,  -height / 2, -length / 2,
      -width / 2,  height / 2,  -length / 2, width / 2,   height / 2,
      -length / 2, width / 2,   -height / 2, -length / 2, -width / 2,
      height / 2,  -length / 2, -width / 2,  height / 2,  length / 2,
      width / 2,   height / 2,  length / 2,  width / 2,   height / 2,
      -length / 2, -width / 2,  -height / 2, -length / 2, width / 2,
      -height / 2, -length / 2, width / 2,   -height / 2, length / 2,
      -width / 2,  -height / 2, length / 2,  width / 2,   -height / 2,
      -length / 2, width / 2,   height / 2,  -length / 2, width / 2,
      height / 2,  length / 2,  width / 2,   -height / 2, length / 2,
      -width / 2,  -height / 2, -length / 2, -width / 2,  -height / 2,
      length / 2,  -width / 2,  height / 2,  length / 2,  -width / 2,
      height / 2,  -length / 2};

  float texcoords[] = {
      0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
      0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
      1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
      0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f};

  float normals[] = {0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
                     1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  -1.0f, 0.0f,
                     0.0f,  -1.0f, 0.0f,  0.0f,  -1.0f, 0.0f,  0.0f,  -1.0f,
                     0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
                     0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  -1.0f, 0.0f,  0.0f,
                     -1.0f, 0.0f,  0.0f,  -1.0f, 0.0f,  0.0f,  -1.0f, 0.0f,
                     1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
                     0.0f,  1.0f,  0.0f,  0.0f,  -1.0f, 0.0f,  0.0f,  -1.0f,
                     0.0f,  0.0f,  -1.0f, 0.0f,  0.0f,  -1.0f, 0.0f,  0.0f};

  unsigned short indices[36];
  int k = 0;
  for (int i = 0; i < 36; i += 6) {
    indices[i] = 4 * k;
    indices[i + 1] = 4 * k + 1;
    indices[i + 2] = 4 * k + 2;
    indices[i + 3] = 4 * k;
    indices[i + 4] = 4 * k + 2;
    indices[i + 5] = 4 * k + 3;
    k++;
  }

  mesh.vertices =
      (float *)allocator->alloc(allocator->context, sizeof(float) * 24 * 3);
  if (!mesh.vertices)
    return mesh;
  memcpy(mesh.vertices, vertices, 24 * 3 * sizeof(float));

  mesh.texcoords =
      (float *)allocator->alloc(allocator->context, sizeof(float) * 24 * 2);
  if (!mesh.texcoords)
    return mesh;
  memcpy(mesh.texcoords, texcoords, 24 * 2 * sizeof(float));

  mesh.normals =
      (float *)allocator->alloc(allocator->context, sizeof(float) * 24 * 3);
  if (!mesh.normals)
    return mesh;
  memcpy(mesh.normals, normals, 24 * 3 * sizeof(float));

  mesh.indices = (unsigned short *)allocator->alloc(
      allocator->context, sizeof(unsigned short) * 36);
  if (!mesh.indices)
    return mesh;
  memcpy(mesh.indices, indices, 36 * sizeof(unsigned short));

  mesh.vertexCount = 24;
  mesh.triangleCount = 12;
  return mesh;
}
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

SpatialGrid *spatialGrid_create_with_dimensions(MemoryAllocator allocator,
                                                Vector3 minBounds,
                                                Vector3 maxBounds, int spacing,
                                                size_t maxEntities) {
  SpatialGrid *sGrid =
      (SpatialGrid *)allocator.alloc(allocator.context, sizeof(SpatialGrid));

  // GRID DETAILS
  sGrid->spacing = spacing;
  sGrid->bX = (int)(maxBounds.x - minBounds.x);
  sGrid->bY = (int)(maxBounds.y - minBounds.y);
  sGrid->numX = sGrid->bX / spacing;
  sGrid->numY = sGrid->bY / spacing;
  sGrid->capacity = maxEntities;

  // DENSE
  sGrid->spatialDense.items = (SpatialEntry *)allocator.alloc(
      allocator.context, sizeof(SpatialEntry) * maxEntities);
  sGrid->spatialDense.count = 0;
  sGrid->spatialDense.capacity = maxEntities;

  // SPARSE
  size_t gridCells = sGrid->numX * sGrid->numY;
  sGrid->spatialSparse.items =
      (size_t *)allocator.alloc(allocator.context, sizeof(size_t) * gridCells);
  sGrid->spatialSparse.capacity = gridCells;
  sGrid->spatialSparse.count = 0;

  // CELL COUNTS
  sGrid->cellCounts.items =
      (size_t *)allocator.alloc(allocator.context, sizeof(size_t) * gridCells);
  sGrid->cellCounts.capacity = gridCells;
  sGrid->cellCounts.count = 0;

  sGrid->isInitalized = true;
  return sGrid;
}

void spatialGrid_update_dimensions(SpatialGrid *sGrid, Vector3 minBounds,
                                   Vector3 maxBounds, int spacing) {
  assert(!sGrid->isInitalized && "Initialize first before updating");

  sGrid->spacing = spacing;
  sGrid->bX = (int)(maxBounds.x - minBounds.x);
  sGrid->bY = (int)(maxBounds.y - minBounds.y);
  sGrid->numX = sGrid->bX / spacing;
  sGrid->numY = sGrid->bY / spacing;
}
static inline size_t spatialGrid_get_cell_index(const SpatialGrid *sGrid,
                                                Vector3 position) {
  int cellX = (int)(position.x / sGrid->spacing);
  int cellY = (int)(position.y / sGrid->spacing);

  if (cellX < 0 || cellX >= (int)sGrid->numX || cellY < 0 ||
      cellY >= (int)sGrid->numY) {
    return (size_t)-1;
  }

  return cellY * sGrid->numX + cellX;
}

static inline size_t
spatialGrid_get_entry_cell_index(const SpatialGrid *sGrid,
                                 const SpatialEntry *entry) {
  return spatialGrid_get_cell_index(sGrid, entry->position);
}
static void _spatialGrid_populate(SpatialGrid *sGrid, arr_vec3f *positions,
                                  arr_size_t *ids) {
  size_t numY = sGrid->numY;

  sGrid->spatialDense.count = 0;
  memset(sGrid->spatialSparse.items, 0,
         sizeof(size_t) * sGrid->spatialSparse.capacity);
  if (sGrid->cellCounts.items) {
    memset(sGrid->cellCounts.items, 0,
           sizeof(size_t) * sGrid->cellCounts.capacity);
  }

  // PART ONE: fill sparse with the entitie counts AND push into dense
  size_t count = 0;
  for (size_t i = 0; i < positions->count; i++) {
    SpatialEntry entry =
        (SpatialEntry){.entityId = ids->items[i],
                       .position = positions->items[i],
                       .entityPoolIndex = 0}; // TODO: for now 0 not used yet
    size_t sparseID = spatialGrid_get_entry_cell_index(sGrid, &entry);
    if (sparseID == (size_t)-1)
      continue;

    sGrid->spatialDense.items[count] = entry;
    sGrid->spatialSparse.items[sparseID] = count + 1;
    count++;
  }
  sGrid->spatialDense.count = count;

  // PART TWO: partial sums
  size_t currentSum = 0;
  for (size_t i = 0; i < sGrid->spatialSparse.capacity; i++) {
    size_t num = sGrid->spatialSparse.items[i];
    if ((currentSum != num) && (num != 0)) {
      currentSum = num;
    }
    sGrid->spatialSparse.items[i] = currentSum;
  }

  // PART THREE: iterate trough dense entries and decrement sparse
  for (size_t i = 0; i < sGrid->spatialDense.count; i++) {
    SpatialEntry *entry = &sGrid->spatialDense.items[i];
    size_t sparseID = spatialGrid_get_entry_cell_index(sGrid, entry);

    sGrid->spatialSparse.items[sparseID]--;
  }
}

void spatial_populate(SpatialGrid *sGrid, EntityPool *entityPool,
                      MemoryAllocator *allocator) {
  arr_vec3f positions = {0};
  arr_size_t ids = {0};

  positions.items = (Vector3 *)allocator->alloc(
      allocator->context, sizeof(Vector3) * entityPool->entities_dense.count);
  positions.capacity = entityPool->entities_dense.count;
  positions.count = 0;

  ids.items = (size_t *)allocator->alloc(
      allocator->context, sizeof(size_t) * entityPool->entities_dense.count);
  ids.capacity = entityPool->entities_dense.count;
  ids.count = 0;

  for (size_t i = 0; i < entityPool->entities_dense.count; i++) {
    Entity *e = &entityPool->entities_dense.items[i];

    if (!(e->flags & ENTITY_FLAG_HAS_TRANSFORM))
      continue;

    positions.items[positions.count] = e->c_transform.pos;
    ids.items[ids.count] = e->identifier;
    positions.count++;
    ids.count++;
  }

  _spatialGrid_populate(sGrid, &positions, &ids);
}

void render(GameMemory *gameMemory, GameState *gameState) {
  RenderQueue *renderQueue = (RenderQueue *)gameState->transientAllocator.alloc(
      gameState->transientAllocator.context, sizeof(RenderQueue));
  renderQueue->count = 0;
  renderQueue->isMeshReloadRequired = false;

  size_t maxTransforms = 10000;
  Matrix *sphereTransforms = (Matrix *)gameState->transientAllocator.alloc(
      gameState->transientAllocator.context, sizeof(Matrix) * maxTransforms);
  size_t sphereCount = 0;

  gameMemory->renderQueue = renderQueue;

  for (size_t i = 0; i < gameState->entityPool->entities_dense.count &&
                     sphereCount < maxTransforms;
       i++) {
    Entity *e = &gameState->entityPool->entities_dense.items[i];
    if ((e->flags & ENTITY_FLAG_VISIBLE) &&
        (e->flags & ENTITY_FLAG_HAS_RENDER)) {
      Matrix t = MatrixTranslate(e->c_transform.pos.x, e->c_transform.pos.y,
                                 e->c_transform.pos.z);
      float scaleFactor = e->c_render.renderRadius * 2.0f;
      Matrix s = MatrixScale(scaleFactor, scaleFactor, scaleFactor);
      sphereTransforms[sphereCount++] = MatrixMultiply(s, t);
    }
  }
  // Push instanced render command
  if (sphereCount > 0) {
    renderQueue->instanceMesh = &gameState->instancedMesh;
    // TODO: workaround. sometimes intanceMesh VAO etc is 0
    //  not sure why.
    if (gameState->instancedMeshUpdated ||
        gameState->instancedMesh.vaoId == 0) {
      renderQueue->isMeshReloadRequired = true;
      gameState->instancedMeshUpdated = false;
    }
    RenderCommand cmd = {RENDER_INSTANCED,
                         .instance = {renderQueue->instanceMesh,
                                      sphereTransforms, NULL, sphereCount}};
    push_render_command(renderQueue, cmd);
  }

  // ==================== BOUNDS DEBUG RENDERING ====================
  RenderCommand cubeCmd = {
      RENDER_CUBE_3D,
      .cube3D = {false, 1, gameState->maxBounds.x, gameState->maxBounds.y,
                 gameState->maxBounds.z,
                 (Color){255, 0, 0, 50}}}; // wireFrame=false, origin=0
                                           // (center), dimensions, color
  push_render_command(renderQueue, cubeCmd);

  Color boundsColorX = (Color){255, 0, 0, 200};
  Color boundsColorY = (Color){0, 0, 255, 200};
  Color boundsColorZ = (Color){0, 255, 0, 200};

  Vector3 min = gameState->minBounds;
  Vector3 max = gameState->maxBounds;
  // Bottom face
  push_render_command(
      renderQueue,
      (RenderCommand){RENDER_LINE_3D, .line3D = {(Vector3){min.x, min.y, min.z},
                                                 (Vector3){max.x, min.y, min.z},
                                                 boundsColorX}});
  push_render_command(
      renderQueue,
      (RenderCommand){RENDER_LINE_3D, .line3D = {(Vector3){max.x, min.y, min.z},
                                                 (Vector3){max.x, max.y, min.z},
                                                 boundsColorX}});
  push_render_command(
      renderQueue,
      (RenderCommand){RENDER_LINE_3D, .line3D = {(Vector3){max.x, max.y, min.z},
                                                 (Vector3){min.x, max.y, min.z},
                                                 boundsColorY}});
  push_render_command(
      renderQueue,
      (RenderCommand){RENDER_LINE_3D, .line3D = {(Vector3){min.x, max.y, min.z},
                                                 (Vector3){min.x, min.y, min.z},
                                                 boundsColorY}});
  // Top face
  push_render_command(
      renderQueue,
      (RenderCommand){RENDER_LINE_3D, .line3D = {(Vector3){min.x, min.y, max.z},
                                                 (Vector3){max.x, min.y, max.z},
                                                 boundsColorZ}});
  push_render_command(
      renderQueue,
      (RenderCommand){RENDER_LINE_3D, .line3D = {(Vector3){max.x, min.y, max.z},
                                                 (Vector3){max.x, max.y, max.z},
                                                 boundsColorZ}});
  push_render_command(
      renderQueue,
      (RenderCommand){RENDER_LINE_3D, .line3D = {(Vector3){max.x, max.y, max.z},
                                                 (Vector3){min.x, max.y, max.z},
                                                 boundsColorX}});
  push_render_command(
      renderQueue,
      (RenderCommand){RENDER_LINE_3D, .line3D = {(Vector3){min.x, max.y, max.z},
                                                 (Vector3){min.x, min.y, max.z},
                                                 boundsColorX}});
  // Vertical edges
  push_render_command(
      renderQueue,
      (RenderCommand){RENDER_LINE_3D, .line3D = {(Vector3){min.x, min.y, min.z},
                                                 (Vector3){min.x, min.y, max.z},
                                                 boundsColorY}});
  push_render_command(
      renderQueue,
      (RenderCommand){RENDER_LINE_3D, .line3D = {(Vector3){max.x, min.y, min.z},
                                                 (Vector3){max.x, min.y, max.z},
                                                 boundsColorX}});
  push_render_command(
      renderQueue,
      (RenderCommand){RENDER_LINE_3D, .line3D = {(Vector3){max.x, max.y, min.z},
                                                 (Vector3){max.x, max.y, max.z},
                                                 boundsColorZ}});
  push_render_command(
      renderQueue,
      (RenderCommand){RENDER_LINE_3D, .line3D = {(Vector3){min.x, max.y, min.z},
                                                 (Vector3){min.x, max.y, max.z},
                                                 boundsColorY}});

  // ==================== SPATIAL GRID DEBUG RENDERING ====================
  if (gameState->sGrid && gameState->sGrid->isInitalized) {
    float spacing = (float)gameState->sGrid->spacing;
    Vector3 min = gameState->minBounds;
    Vector3 max = gameState->maxBounds;
    int numX = gameState->sGrid->numX;
    int numY = gameState->sGrid->numY;
    Color gridColor = (Color){100, 255, 100, 255};

    // Vertical grid lines
    for (int x = 0; x <= numX; x++) {
      float gx = min.x + x * spacing;
      push_render_command(
          renderQueue,
          (RenderCommand){RENDER_LINE_3D,
                          .line3D = {(Vector3){gx, min.y, min.z},
                                     (Vector3){gx, max.y, min.z}, gridColor}});
      push_render_command(
          renderQueue,
          (RenderCommand){RENDER_LINE_3D,
                          .line3D = {(Vector3){gx, min.y, max.z},
                                     (Vector3){gx, max.y, max.z}, gridColor}});
    }
    // Horizontal grid lines
    for (int y = 0; y <= numY; y++) {
      float gy = min.y + y * spacing;
      push_render_command(
          renderQueue,
          (RenderCommand){RENDER_LINE_3D,
                          .line3D = {(Vector3){min.x, gy, min.z},
                                     (Vector3){max.x, gy, min.z}, gridColor}});
      push_render_command(
          renderQueue,
          (RenderCommand){RENDER_LINE_3D,
                          .line3D = {(Vector3){min.x, gy, max.z},
                                     (Vector3){max.x, gy, max.z}, gridColor}});
    }
    // // Cell centers
    // for (int x = 0; x < numX; x++) {
    //   for (int y = 0; y < numY; y++) {
    //     // CELL CENTER
    //     float cellCenterX = min.x + (x + 0.5f) * spacing;
    //     float cellCenterY = min.y + (y + 0.5f) * spacing;
    //     float cellCenterZ = max.z;
    //     float sphereRadius = 3.0f;
    //     Color cellColor = (Color){(25 * x) % 255, (25 * y) % 255, 0, 200};
    //
    //     push_render_command(
    //         renderQueue,
    //         (RenderCommand){
    //             RENDER_SPHERE_3D,
    //             .sphere3D = {(Vector3){cellCenterX, cellCenterY,
    //             cellCenterZ},
    //                          sphereRadius, cellColor}});
    //   }
    // }
  }
}
void handle_input(GameState *gameState, Input *input);
void handle_update(GameState *gameState, float frameTime, Input *input);
void handle_init(GameMemory *gameMemory, GameState *gameState);
void update_collision(GameState *gameState, float frameTime);

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
