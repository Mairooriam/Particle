#pragma once
#include "shared.h"
#include "application_types.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
// application.c

static inline Vector3 Vector3Add(Vector3 v1, Vector3 v2) {
  return (Vector3){v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
}

static inline Vector2 Vector2Subtract(Vector2 v1, Vector2 v2) {
  return (Vector2){v1.x - v2.x, v1.y - v2.y};
}

static inline Vector3 Vector3Scale(Vector3 v, float scale) {
  return (Vector3){v.x * scale, v.y * scale, v.z * scale};
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

void update_entity_position(Entity *e, float frameTime,
                            Vector2 mouseWorldPosition);
void update_entity_boundaries(Entity *e, float x_bound, float x_bound_min,
                              float y_bound, float y_bound_min, float z_bound,
                              float z_bound_min);
Entity entity_create_physics_particle(Vector3 pos, Vector3 velocity);
Entity entity_create_spawner_entity();

void push_render_command(RenderQueue *queue, RenderCommand cmd);

static inline void Entities_init_with_buffer(Entities *da, size_t cap,
                                             Entity *buffer) {
  da->count = 0;
  da->capacity = cap;
  da->items = buffer;
}

// TODO: make entities_dense be entities Dynamic array so it has count when
// passed around?

void update_spawners(float frameTime, Entity *e, EntityPool *entityPool);
#define memory_index                                                           \
  size_t // from casey experiment and see if this is good way to do this

void arena_init(memory_arena *arena, uint8_t *base, memory_index arena_size) {
  arena->base = base;
  arena->size = arena_size;
  arena->used = 0;
}

#define arena_PushStruct(Arena, type) (type *)_PushStruct(Arena, sizeof(type))
#define arena_PushStructs(Arena, type, count)                                  \
  (type *)_PushStruct(Arena, sizeof(type) * count)
// Ment for internal use do not call
// Zeroes out the pushed struct
static void *_PushStruct(memory_arena *arena, size_t size) {
  Assert(arena->used + size <= arena->size);
  void *result = arena->base + arena->used;
  arena->used += size;
  return result;
}

// Initialize the entity pool
static EntityPool *EntityPoolInitInArena(memory_arena *arena, size_t capacity) {
  EntityPool *pool = arena_PushStruct(arena, EntityPool);
  pool->capacity = capacity;

  pool->entities_sparse.items = (size_t *)arena_PushStructs(
      arena, size_t, capacity + 1); // +1 for unused index 0
  pool->entities_sparse.capacity = capacity + 1;
  pool->entities_sparse.count = 0;

  pool->entities_dense.items =
      (Entity *)arena_PushStructs(arena, Entity, capacity);
  pool->entities_dense.capacity = capacity;
  pool->entities_dense.count = 0;

  pool->freeIds.items =
      (size_t *)arena_PushStructs(arena, size_t, capacity); // Free ID stack
  pool->freeIds.capacity = capacity;
  pool->freeIds.count = 0;

  pool->nextId = 1; // Start at 1

  for (size_t i = 0; i <= pool->entities_sparse.capacity;
       i++) { // Include index 0 (though not used)
    pool->entities_sparse.items[i] = SIZE_MAX;
  }

  return pool;
}
void PrintSparseAndDense(EntityPool *pool, size_t start, size_t end) {
  if (start >= pool->capacity) {
    printf("Error: Start index out of bounds.\n");
    return;
  }
  if (end > pool->capacity) {
    end = pool->capacity; // Clamp the end index to the capacity
  }

  printf("Sparse Array (from %zu to %zu):\n", start, end);
  for (size_t i = start; i < end; i++) {
    printf("Sparse[%zu] = %zu\n", i, pool->entities_sparse.items[i]);
  }

  printf("\nDense Array (from %zu to %zu):\n", start, end);
  for (size_t i = start; i < end && i < pool->entities_dense.count; i++) {
    printf("Dense[%zu] = { ID: %zu }\n", i, pool->entities_dense.items[i].id);
  }
  printf("\n");
}
static size_t EntityPoolGetNextId(EntityPool *pool) {
  if (pool->freeIds.count > 0) {
    return pool->freeIds.items[--pool->freeIds.count];
  } else {
    return pool->nextId++; // return new id
  }
}

static void EntityPoolPush(EntityPool *pool, Entity entity) {
  // If supplied entity ID == 0 -> choose id for it
  if (entity.id == 0) {
    entity.id = EntityPoolGetNextId(pool);
  } else {
    // If ID is pre-assigned, ensure it's free and valid
    // TODO: do i want to add flag to override exisiting? or do i want to delete
    // it first and then push new one?
    Assert(entity.id <= pool->capacity);
    Assert(pool->entities_sparse.items[entity.id] ==
           SIZE_MAX); // Ensure ID slot is free (prevent duplicates)
  }
  // Assert incase someone changes entity id to be not unsigned
  Assert(entity.id >= 0)

      // currently pool doesnt grow. assert entity is fits the pool
      Assert(entity.id <= pool->capacity);
  if (pool->entities_dense.count < pool->capacity) {
    // Add to the end of the dense array
    pool->entities_dense.items[pool->entities_dense.count] = entity;
    pool->entities_sparse.items[entity.id] =
        pool->entities_dense.count; // Use entity.id directly
    pool->entities_dense.count++;
  } else {
    printf("Error: Entity capacity reached.\n");
  }
}

static void EntityPoolRemoveIdx(EntityPool *pool, size_t idx) {
  // Safety check: Ensure the pool has entities and the ID is valid/active
  if (pool->entities_dense.count == 0) {
    // printf("Warning: Attempted to remove from an empty EntityPool.\n");
    return;
  }
  if (idx == 0 || idx > pool->capacity) {
    // printf("Warning: Invalid ID %zu for removal (out of range).\n", idx);
    return;
  }
  size_t denseIndex = pool->entities_sparse.items[idx]; // Use idx directly
  if (denseIndex == SIZE_MAX || denseIndex >= pool->entities_dense.count) {
    // printf("Warning: ID %zu is not active or invalid for removal.\n", idx);
    return;
  }

  // Swap with the last entity (even if it's the same)
  pool->entities_dense.items[denseIndex] =
      pool->entities_dense.items[pool->entities_dense.count - 1];
  size_t swappedId = pool->entities_dense.items[denseIndex].id;
  pool->entities_sparse.items[swappedId] = denseIndex; // Use swappedId directly

  // Mark removed ID as free
  pool->entities_sparse.items[idx] = SIZE_MAX; // Use idx directly
  pool->freeIds.items[pool->freeIds.count++] = idx;
  pool->entities_dense.count--;
}

typedef struct {
  Input lastFrameInput;
  Vector2 mouseDelta;
  Mesh instancedMesh;
  Input inputLastFrame;
  bool instancedMeshUpdated;
  memory_arena permanentArena;
  memory_arena transientArena;
  EntityPool *entityPool; // stores entities
  SpatialGrid *sGrid;
  Vector3 minBounds;
  Vector3 maxBounds;
} GameState;
static Mesh GenMeshCube(memory_arena *arena, float width, float height,
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

  mesh.vertices = arena_PushStructs(arena, float, 24 * 3);
  if (!mesh.vertices)
    return mesh; // Allocation failed
  memcpy(mesh.vertices, vertices, 24 * 3 * sizeof(float));

  mesh.texcoords = arena_PushStructs(arena, float, 24 * 2);
  if (!mesh.texcoords)
    return mesh;
  memcpy(mesh.texcoords, texcoords, 24 * 2 * sizeof(float));

  mesh.normals = arena_PushStructs(arena, float, 24 * 3);
  if (!mesh.normals)
    return mesh;
  memcpy(mesh.normals, normals, 24 * 3 * sizeof(float));

  mesh.indices = arena_PushStructs(arena, unsigned short, 36);
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

SpatialGrid *spatialGrid_create_with_dimensions(memory_arena *arena,
                                                Vector3 minBounds,
                                                Vector3 maxBounds, int spacing,
                                                size_t maxEntities) {
  SpatialGrid *sGrid = arena_PushStruct(arena, SpatialGrid);

  // Calculate grid dimensions first
  sGrid->spacing = spacing;
  sGrid->bX = (int)(maxBounds.x - minBounds.x);
  sGrid->bY = (int)(maxBounds.y - minBounds.y);
  sGrid->numX = sGrid->bX / spacing;
  sGrid->numY = sGrid->bY / spacing;

  size_t gridCells = sGrid->numX * sGrid->numY;
  sGrid->capacity = maxEntities;

  sGrid->spatialDense.items = arena_PushStructs(arena, size_t, maxEntities);
  sGrid->spatialDense.count = 0;
  sGrid->spatialDense.capacity = maxEntities;

  sGrid->spatialSparse.items = arena_PushStructs(arena, size_t, gridCells);
  sGrid->spatialSparse.capacity = gridCells; // NOT maxEntities!
  sGrid->spatialSparse.count = 0;

  sGrid->isInitalized = true;

  return sGrid;
}
void spatialGrid_update_dimensions(SpatialGrid *sGrid, Vector3 minBounds,
                                   Vector3 maxBounds, int spacing) {
  sGrid->spacing = spacing;
  sGrid->bX = (int)(maxBounds.x - minBounds.x);
  sGrid->bY = (int)(maxBounds.y - minBounds.y);
  sGrid->numX = sGrid->bX / spacing;
  sGrid->numY = sGrid->bY / spacing;
  sGrid->isInitalized = true;
}
void update_spatial(SpatialGrid *sGrid, arr_Entity *e) {
  float spacing = sGrid->spacing;
  // MEMORY SAFETY CHECK - Add this assert to catch overlap

  // Clear arrays
  memset(sGrid->spatialSparse.items, 0,
         sGrid->spatialSparse.capacity * sizeof(sGrid->spatialSparse.items[0]));
  sGrid->spatialDense.count = 0;

  // First pass: count entities per cell
  for (size_t i = 0; i < e->count; i++) {
    c_Transform *cT1 = &e->items[i].c_transform;

    float xi_f = cT1->pos.x / spacing;
    float yi_f = cT1->pos.y / spacing;
    if (xi_f < 0 || yi_f < 0)
      continue;

    size_t xi = (size_t)floorf(xi_f);
    size_t yi = (size_t)floorf(yi_f);

    if (xi >= sGrid->numX || yi >= sGrid->numY)
      continue;

    size_t idx = xi * sGrid->numY + yi;
    if (idx >= sGrid->spatialSparse.capacity)
      continue;

    // Count entities in this cell
    sGrid->spatialSparse.items[idx]++;
  }

  // Prefix sum to get start indices
  size_t sum = 0;
  for (size_t i = 0; i < sGrid->spatialSparse.capacity; i++) {
    size_t count = sGrid->spatialSparse.items[i];
    sGrid->spatialSparse.items[i] = sum; // Store start index
    sum += count;
  }

  // Resize dense array to fit all entities
  // sGrid->antitiesDense.count = sum;

  // Second pass: fill dense array
  for (size_t i = 0; i < e->count; i++) {
    c_Transform *cT1 = &e->items[i].c_transform;

    float xi_f = cT1->pos.x / spacing;
    float yi_f = cT1->pos.y / spacing;
    if (xi_f < 0 || yi_f < 0)
      continue;

    size_t xi = (size_t)floorf(xi_f);
    size_t yi = (size_t)floorf(yi_f);

    if (xi >= sGrid->numX || yi >= sGrid->numY)
      continue;

    size_t idx = xi * sGrid->numY + yi;
    if (idx >= sGrid->spatialSparse.capacity)
      continue;

    // Add entity to dense array at the next available position for this cell
    size_t denseIdx = sGrid->spatialSparse.items[idx];
    sGrid->spatialDense.items[denseIdx] = i;
    sGrid->spatialSparse.items[idx]++; // Increment for next entity in this cell
  }

  // Restore start indices (optional, needed for querying)
  sum = 0;
  for (size_t i = 0; i < sGrid->spatialSparse.capacity; i++) {
    size_t end = sGrid->spatialSparse.items[i];
    sGrid->spatialSparse.items[i] = sum;
    sum = end;
  }

  sGrid->spatialDense.count = sum;
  sGrid->spatialSparse.count = sGrid->spatialSparse.capacity;
}

void render(GameMemory *gameMemory, GameState *gameState) {
  // Render entities (instanced)
  RenderQueue *renderQueue = (RenderQueue *)gameMemory->transientMemory;
  renderQueue->count = 0;

  // Allocate transforms in transient memory
  Matrix *sphereTransforms =
      (Matrix *)((char *)gameMemory->transientMemory + sizeof(RenderQueue));
  size_t maxTransforms =
      (gameMemory->transientMemorySize - sizeof(RenderQueue)) / sizeof(Matrix);
  size_t sphereCount = 0;

  for (size_t i = 0; i < gameState->entityPool->entities_dense.count &&
                     sphereCount < maxTransforms;
       i++) {
    Entity *e = &gameState->entityPool->entities_dense.items[i];
    if ((e->flags & ENTITY_FLAG_VISIBLE) &&
        (e->flags & ENTITY_FLAG_HAS_RENDER)) {
      // Collect transform for instanced drawing
      Matrix t = MatrixTranslate(e->c_transform.pos.x, e->c_transform.pos.y,
                                 e->c_transform.pos.z);
      Matrix s =
          MatrixScale(1.1f, 1.1f, 1.1f); // Change 2.0f to your desired scale
      sphereTransforms[sphereCount++] = MatrixMultiply(s, t);
    }
  }
  // Push instanced render command
  if (sphereCount > 0) {
    if (gameState->instancedMeshUpdated) {
      renderQueue->isMeshReloadRequired = true;
      renderQueue->instanceMesh = gameState->instancedMesh;
      gameState->instancedMeshUpdated = false;
    }
    RenderCommand cmd = {RENDER_INSTANCED,
                         .instance = {&renderQueue->instanceMesh,
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
    // Cell centers
    for (int x = 0; x < numX; x++) {
      for (int y = 0; y < numY; y++) {
        // CELL CENTER
        float cellCenterX = min.x + (x + 0.5f) * spacing;
        float cellCenterY = min.y + (y + 0.5f) * spacing;
        float cellCenterZ = max.z;
        float sphereRadius = 3.0f;
        Color cellColor = (Color){(25 * x) % 255, (25 * y) % 255, 0, 200};

        push_render_command(
            renderQueue,
            (RenderCommand){
                RENDER_SPHERE_3D,
                .sphere3D = {(Vector3){cellCenterX, cellCenterY, cellCenterZ},
                             sphereRadius, cellColor}});
      }
    }
  }
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
