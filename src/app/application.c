// application.c
#include "application.h"
#include "app/application_types.h"
#include "cglm/mat4.h"
#include "cglm/vec2.h"
#include "entityPool_types.h"
#include "entity_types.h"
#include "memory_allocator.h"
#include "shared.h"
#include "stdio.h"
#include <math.h>
#include <oaidl.h>
// if moving to c++ to prevent name mangling
// extern "C" GAME_UPDATE(game_update) { pos->y++; }

static inline arr_mat4 *arr_mat4_create(size_t capacity,
                                        MemoryAllocator *allocator) {
  arr_mat4 *da =
      (arr_mat4 *)allocator->alloc(allocator->context, sizeof(arr_mat4));
  if (!da)
    return NULL;

  da->items =
      (mat4 *)allocator->alloc(allocator->context, sizeof(mat4) * capacity);
  if (!da->items) {
    allocator->free(allocator->context, da);
    return NULL;
  }

  da->capacity = capacity;
  da->count = 0;
  return da;
}

static inline void arr_mat4_free(arr_mat4 *da, MemoryAllocator *allocator) {
  if (da) {
    allocator->free(allocator->context, da->items);
    allocator->free(allocator->context, da);
  }
}

static inline void arr_mat4_resize(arr_mat4 *da, size_t new_capacity,
                                   MemoryAllocator *allocator) {
  if (new_capacity > da->capacity) {
    mat4 *new_items = (mat4 *)allocator->alloc(allocator->context,
                                               sizeof(mat4) * new_capacity);
    if (!new_items)
      return;

    memcpy(new_items, da->items, sizeof(mat4) * da->count);
    allocator->free(allocator->context, da->items);
    da->items = new_items;
    da->capacity = new_capacity;
  }
}

static inline void arr_mat4_append(arr_mat4 *da, const mat4 *item,
                                   MemoryAllocator *allocator) {
  if (da->count >= da->capacity) {
    arr_mat4_resize(da, da->capacity * 2, allocator);
  }
  memcpy(&da->items[da->count], item, sizeof(mat4));
  da->count++;
}

void mat4_lerp(mat4 result, const mat4 a, const mat4 b, float t) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      result[i][j] = a[i][j] * (1.0f - t) + b[i][j] * t;
    }
  }
}
GAME_UPDATE(game_update) {
  Assert(sizeof(GameState) <= gameMemory->permanentMemorySize);

  // ==================== INITIALIZATION ====================
  GameState *gameState = (GameState *)gameMemory->permamentMemory;
  if (!gameMemory->isInitialized) {
    handle_init(gameMemory, gameState);
    gameState->p[0] = 0;
    gameState->p[1] = 0;
    gameState->v[0] = 0.005f;
    gameState->v[1] = 0.001f;
  }
  if (gameMemory->reloadDLLHappened) {
    gameState->permanentAllocator.alloc = arena_alloc;
    gameState->permanentAllocator.free = arena_free;
    gameState->transientAllocator.alloc = arena_alloc;
    gameState->transientAllocator.free = arena_free;

    gameState->v[0] = 0.005f * 2;
    gameState->v[1] = 0.001f * 10;
    printf("reload dll happened, updated variables\n");
    gameMemory->reloadDLLHappened = false;
  }

  arena_reset(&gameState->transientArena);

  handle_input(gameState, input);
  glm_vec2_add(gameState->p, gameState->v, gameState->p);

  printf("Position: (%f, %f), Velocity: (%f, %f)\n", gameState->p[0],
         gameState->p[1], gameState->v[0], gameState->v[1]);
  arr_mat4 *transforms = arr_mat4_create(5, &gameState->transientAllocator);

  mat4 identity;
  glm_mat4_identity(identity);

  mat4 rotation;
  glm_mat4_zero(rotation);
  float rot = M_PI / 2;
  rotation[0][0] = cosf(rot);
  rotation[0][1] = -sinf(rot);
  rotation[1][0] = sinf(rot);
  rotation[1][1] = cosf(rot);
  rotation[2][2] = 1.0f;
  rotation[3][3] = 1.0f;

  mat4 translate;
  glm_mat4_zero(translate);
  translate[0][0] = 1;
  translate[1][1] = 1;
  translate[2][2] = 1.0f;
  translate[3][3] = 1.0f;
  translate[3][0] = gameState->p[0];
  translate[3][1] = gameState->p[1];
  translate[3][2] = 0;

  mat4 scale;
  glm_mat4_zero(scale);
  scale[0][0] = 1.5;
  scale[1][1] = 2.5;
  scale[2][2] = 1.0f;
  scale[3][3] = 1.0f;

  mat4 shear;
  glm_mat4_zero(shear);
  shear[0][0] = 1.0f;
  shear[0][1] = 0.5f;
  shear[1][1] = 1.0f;
  shear[2][2] = 1.0f;
  shear[3][3] = 1.0f;

  static float t = 0.0f;
  t += 0.005f;
  if (t > 1.0f)
    t = 0.0f;

  mat4 scaleLerp;
  mat4_lerp(scaleLerp, identity, scale, t);
  // arr_mat4_append(transforms, &scale, &gameState->transientAllocator);
  //
  mat4 rotationLerp;
  mat4_lerp(rotationLerp, identity, rotation, t);
  // arr_mat4_append(transforms, &rotation, &gameState->transientAllocator);
  //
  mat4 shearLerp;
  mat4_lerp(shearLerp, identity, shear, t);
  // arr_mat4_append(transforms, &shear, &gameState->transientAllocator);

  // Combine all transforms (scale, rotation, shear)
  mat4 tempTransform1, tempTransform2, tempTransform3, combinedTransform;
  // glm_mat4_mul(scaleLerp, rotationLerp, tempTransform1);
  // glm_mat4_mul(tempTransform1, shearLerp, tempTransform2);
  // glm_mat4_mul(tempTransform2, scaleLerp, tempTransform3);
  // mat4_lerp(combinedTransform, identity, rotationLerp, t);
  // arr_mat4_append(transforms, &combinedTransform,
  //                 &gameState->transientAllocator);

  glm_mat4_mul(scale, rotation, tempTransform1);
  glm_mat4_mul(tempTransform1, shear, tempTransform2);
  glm_mat4_mul(tempTransform2, translate, tempTransform3);

  glm_mat4_copy(tempTransform3, combinedTransform);

  arr_mat4_append(transforms, &combinedTransform,
                  &gameState->transientAllocator);

  gameMemory->transforms = transforms;

  if (gameState->p[0] <= -1) {
    gameState->v[0] = -gameState->v[0];
    gameState->p[0] = -1;
  } else if (gameState->p[0] >= 1) {
    gameState->v[0] = -gameState->v[0];
    gameState->p[0] = 1;
  }

  if (gameState->p[1] >= 1) {
    gameState->p[1] = 1;
    gameState->v[1] = -gameState->v[1];

  } else if (gameState->p[1] <= -1) {
    gameState->p[1] = -1;
    gameState->v[1] = -gameState->v[1];
  }
  vec4 *instanceColors = (vec4 *)gameState->transientAllocator.alloc(
      gameState->transientAllocator.context, sizeof(vec4) * transforms->count);

  if (transforms->count > 0) {
    instanceColors[0][0] = 1.0f;
    instanceColors[0][1] = 0.0f;
    instanceColors[0][2] = 0.0f;
    instanceColors[0][3] = 1.0f;
  }
  if (transforms->count > 1) {
    instanceColors[1][0] = 0.0f;
    instanceColors[1][1] = 0.65f;
    instanceColors[1][2] = 0.0f;
    instanceColors[1][3] = 0.5f;
  }
  if (transforms->count > 2) {
    instanceColors[2][0] = 0.5f;
    instanceColors[2][1] = 0.5f;
    instanceColors[2][2] = 0.5f;
    instanceColors[2][3] = 0.5f;
  }
  if (transforms->count > 3) {
    instanceColors[3][0] = 0.0f;
    instanceColors[3][1] = 1.0f;
    instanceColors[3][2] = 1.0f;
    instanceColors[3][3] = 0.5f;
  }
  gameMemory->instanceColors = instanceColors;

  // handle_update(gameState, frameTime, input);
  // render(gameMemory, gameState);

  gameState->lastFrameInput = *input;
}

void update_spawners(float frameTime, Entity *e, EntityPool *entityPool) {
  e->clock += frameTime;
  if (e->clock > (1 / e->spawnRate)) {
    for (size_t i = 0; i < e->spawnCount; i++) {
      e->spawnEntity->c_transform.pos = e->c_transform.pos;
      entityPool_push(entityPool, *e->spawnEntity);
    }
    e->clock = 0.0f;
  }
}
void push_render_command(RenderQueue *queue, RenderCommand cmd) {
  if (queue->count < MAX_RENDER_COMMANDS) {
    queue->commands[queue->count++] = cmd;
  }
}

void update_entity_position(Entity *e, float frameTime, Vector2 mouseWorldPos) {
  c_Transform *cTp1 = &e->c_transform;

  if (e->followMouse) {
    float lerpFactor = 0.1f;
    cTp1->pos.x = Lerp(cTp1->pos.x, mouseWorldPos.x, lerpFactor);
    cTp1->pos.y = Lerp(cTp1->pos.y, mouseWorldPos.y, lerpFactor);
    cTp1->pos.z = 0.0f;

    // TODO: disable physics while following to avoid interference maybe?!?
    cTp1->v = (Vector3){0, 0, 0};
    cTp1->a = (Vector3){0, 0, 0};
  } else {
    // Normal physics update
    cTp1->v = Vector3Add(cTp1->v, Vector3Scale(cTp1->a, frameTime));
    cTp1->pos = Vector3Add(cTp1->pos, Vector3Scale(cTp1->v, frameTime));
  }
}
void update_entity_boundaries(Entity *e, float x_bound, float x_bound_min,
                              float y_bound, float y_bound_min, float z_bound,
                              float z_bound_min) {
  c_Transform *cTp1 = &e->c_transform;

  // X bounds
  if (cTp1->pos.x < x_bound_min) {
    cTp1->pos.x = x_bound_min;
    cTp1->v.x = cTp1->v.x * -1;
  } else if (cTp1->pos.x > x_bound) {
    cTp1->pos.x = x_bound;
    cTp1->v.x = cTp1->v.x * -1;
  }

  // Y bounds
  if (cTp1->pos.y < y_bound_min) {
    cTp1->pos.y = y_bound_min;
    cTp1->v.y = cTp1->v.y * -1;
  } else if (cTp1->pos.y > y_bound) {
    cTp1->pos.y = y_bound;
    cTp1->v.y = cTp1->v.y * -1;
  }

  // Z bounds
  if (cTp1->pos.z < z_bound_min) {
    cTp1->pos.z = z_bound_min;
    cTp1->v.z = cTp1->v.z * -1;
  } else if (cTp1->pos.z > z_bound) {
    cTp1->pos.z = z_bound;
    cTp1->v.z = z_bound;
    cTp1->v.z = cTp1->v.z * -1;
  }
}
void handle_input(GameState *gameState, Input *input) {
  gameState->mouseDelta =
      Vector2Subtract(input->mousePos, gameState->lastFrameInput.mousePos);

  if (is_key_pressed(input, &gameState->lastFrameInput, KEY_SPACE)) {
    // NUKE half of entitieseses
    static size_t idx = 0;
    for (size_t i = 0; i < gameState->entityPool->entities_dense.count; i++) {
      Entity *e = &gameState->entityPool->entities_dense.items[i];
      if (idx % 2) {
        entityPool_remove(gameState->entityPool, e->identifier);
      }
      idx++;
    }
    printf("Space pressed!\n");
  }

  if (is_key_released(input, &gameState->lastFrameInput, KEY_1)) {

    // PrintSparseAndDense(gameState->entityPool, 0, 50);
    printf("Space released!\n");
  }

  if (is_key_down(input, KEY_SPACE)) {
    printf("Space is down!\n");
  }
}
void handle_update(GameState *gameState, float frameTime, Input *input) {
  EntityPool *entityPool = gameState->entityPool;
  // Update entities

  update_collision(gameState, frameTime);

  for (size_t i = 0; i < entityPool->entities_dense.count; i++) {
    Entity *e = &entityPool->entities_dense.items[i];
    if (e->flags & ENTITY_FLAG_ACTIVE) {
      if (e->flags & ENTITY_FLAG_HAS_TRANSFORM) {
        e->c_transform.a = (Vector3){0, 9.81, 0};
        update_entity_position(e, frameTime, input->mousePos);
        update_entity_boundaries(e, gameState->maxBounds.x,
                                 gameState->minBounds.x, gameState->maxBounds.y,
                                 gameState->minBounds.y, gameState->maxBounds.z,
                                 gameState->minBounds.z);
      }
      if (e->flags & ENTITY_FLAG_HAS_SPAWNER) {
        // e->c_transform.v = (Vector3){10, 10, 0};
        // e->c_transform.v =
        //     Vector3Add(e->c_transform.v, (Vector3){1, 2.0f, 0.5f});
        update_spawners(frameTime, e, entityPool);
      }
    }
  }

  spatial_populate(gameState->sGrid, entityPool,
                   &gameState->transientAllocator);
}
void update_collision(GameState *gameState, float frameTime) {
  (void)frameTime;
  EntityPool *ePool = gameState->entityPool;
  SpatialGrid *sGrid = gameState->sGrid;

  for (size_t i = 0; i < sGrid->spatialDense.count; i++) {
    SpatialEntry *entry = &sGrid->spatialDense.items[i];
    Vector3 eP1 = entry->position;
    int cellX = (int)(eP1.x / sGrid->spacing);
    int cellY = (int)(eP1.y / sGrid->spacing);

    if (cellX < 0 || cellX >= (int)sGrid->numX || cellY < 0 ||
        cellY >= (int)sGrid->numY) {
      continue;
    }
    // spatialGrid_get_entry_cell_index(const SpatialGrid *sGrid, const
    // SpatialEntry *entry)

    // TODO: does this check against its own cell if cellx and celly at 0
    for (int dx = -1; dx <= 1; dx++) {
      for (int dy = -1; dy <= 1; dy++) {
        int nX = cellX + dx;
        int nY = cellY + dy;

        // Check all bounds
        if (nX < 0 || nX >= (int)sGrid->numX || nY < 0 ||
            nY >= (int)sGrid->numY) {
          continue;
        }

        size_t sparseGridID = nY * sGrid->numX + nX;
        size_t idxInDense = sGrid->spatialSparse.items[sparseGridID];
        size_t nextIdx = (sparseGridID + 1 < sGrid->spatialSparse.capacity)
                             ? sGrid->spatialSparse.items[sparseGridID + 1]
                             : sGrid->spatialDense.count;
        size_t amountOfEntitiesInThatCell = nextIdx - idxInDense;

        // Iterate through all entities in this neighboring cell
        for (size_t k = idxInDense; k < idxInDense + amountOfEntitiesInThatCell;
             k++) {
          SpatialEntry *otherEntry = &sGrid->spatialDense.items[k];

          // Skip self-collision
          if (entry->entityId == otherEntry->entityId) {
            continue;
          }

          // Get entities from pool
          Entity *e1 = entityPool_get(ePool, entry->entityId);
          Entity *e2 = entityPool_get(ePool, otherEntry->entityId);

          if (!e1 || !e2)
            continue;

          c_Transform *cTp1 = &e1->c_transform;
          c_Transform *cTp2 = &e2->c_transform;
          c_Collision *cCp1 = &e1->c_collision;
          c_Collision *cCp2 = &e2->c_collision;

          // Collision detection
          if (CheckCollisionCircles(
                  (Vector2){cTp1->pos.x, cTp1->pos.y}, cCp1->radius,
                  (Vector2){cTp2->pos.x, cTp2->pos.y}, cCp2->radius)) {

            // Collision response (same as your original code)
            Vector3 delta = Vector3Subtract(cTp1->pos, cTp2->pos);
            float distance = Vector3Length(delta);
            Vector3 n12 = (distance == 0.0f) ? (Vector3){0.0f, 1.0f, 0.0f}
                                             : Vector3Normalize(delta);

            float overlap = (cCp1->radius + cCp2->radius) - distance;
            float totalInverseMass = cCp1->inverseMass + cCp2->inverseMass;

            if (totalInverseMass > 0) {
              Vector3 separation1 = Vector3Scale(
                  n12, overlap * cCp2->inverseMass / totalInverseMass);
              Vector3 separation2 = Vector3Scale(
                  n12, -overlap * cCp1->inverseMass / totalInverseMass);
              cTp1->pos = Vector3Add(cTp1->pos, separation1);
              cTp2->pos = Vector3Add(cTp2->pos, separation2);
            }

            cCp1->collisionCount++;
            Vector3 dV12 = Vector3Subtract(cTp1->v, cTp2->v);
            float Vs = Vector3DotProduct(dV12, n12);

            if (Vs <= 0) {
              float nVs = -Vs * cTp1->restitution;
              float deltaV = nVs - Vs;
              float impulse = deltaV / totalInverseMass;
              Vector3 impulsePerIMass = Vector3Scale(n12, impulse);

              cTp1->v = Vector3Add(
                  cTp1->v, Vector3Scale(impulsePerIMass, cCp1->inverseMass));
              cTp2->v = Vector3Subtract(
                  cTp2->v, Vector3Scale(impulsePerIMass, cCp2->inverseMass));
            }
          }
        }
      }
    }

    // for (size_t i = 0; i < ePool->entities_dense.count; i++) {
    //   Entity *e = &ePool->entities_dense.items[i];
    //   c_Transform *cTp1 = &e->c_transform;
    //   c_Collision *cCp1 = &e->c_collision;
    //
    //   for (size_t j = 0; j < ePool->entities_dense.count; j++) {
    //     if (i == j)
    //       continue;
    //     Entity *e2 = &ePool->entities_dense.items[j];
    //     c_Transform *cTp2 = &e2->c_transform;
    //     c_Collision *cCp2 = &e2->c_collision;
    //
    //     if (CheckCollisionCircles(
    //             (Vector2){cTp1->pos.x, cTp1->pos.y}, cCp1->radius,
    //             (Vector2){cTp2->pos.x, cTp2->pos.y}, cCp2->radius)) {
    //       Vector3 delta = Vector3Subtract(cTp1->pos, cTp2->pos);
    //       float distance = Vector3Length(delta);
    //       Vector3 n12 = Vector3Normalize(delta);
    //       // If objects are spawned on top of each other delta is 0.0.
    //       if (distance == 0.0f) {
    //         n12 = (Vector3){0.0f, 1.0f, 0.0f}; // YODO: test other kinds of
    //         forces
    //       } else {
    //         n12 = Vector3Normalize(delta);
    //       }
    //       float overlap = (cCp1->radius + cCp2->radius) - distance;
    //       float totalInverseMass = cCp1->inverseMass + cCp2->inverseMass;
    //       if (totalInverseMass <= 0)
    //         break; // infinite mass impulses have no effect
    //
    //       if (totalInverseMass >
    //           0) { // Only separate if at least one has finite mass
    //         Vector3 separation1 =
    //             Vector3Scale(n12, overlap * cCp2->inverseMass /
    //             totalInverseMass);
    //         Vector3 separation2 = Vector3Scale(n12, -overlap *
    //         cCp1->inverseMass /
    //                                                     totalInverseMass);
    //         cTp1->pos = Vector3Add(cTp1->pos, separation1);
    //         cTp2->pos = Vector3Add(cTp2->pos, separation2);
    //       } else {
    //         // Both infinite mass: don't separate (or handle as static)
    //       }
    //       cCp1->collisionCount++;
    //       Vector3 dV12 = Vector3Subtract(cTp1->v, cTp2->v);
    //       float Vs = Vector3DotProduct(dV12, n12); // separation velocity
    //
    //       if (Vs > 0) {
    //         break; // contact is either separating or stationary no impulse
    //                // needed
    //       }
    //       float nVs =
    //           -Vs * cTp1->restitution; // New separation velocity with
    //           restitution
    //       float deltaV = nVs - Vs;
    //
    //       // TODO: not checking if no particle -> colliding with wall? etc?
    //       in
    //       // example there is
    //
    //       float impulse = deltaV / totalInverseMass;
    //
    //       Vector3 impulsePerIMass = Vector3Scale(n12, impulse);
    //
    //       cTp1->v = Vector3Add(cTp1->v,
    //                            Vector3Scale(impulsePerIMass,
    //                            cCp1->inverseMass));
    //       cTp2->v = Vector3Subtract(
    //           cTp2->v, Vector3Scale(impulsePerIMass, cCp2->inverseMass));
    //     }
    //   }
    // }
  }
}

void handle_init(GameMemory *gameMemory, GameState *gameState) {
  size_t gameStateSize = sizeof(GameState);

  size_t entityCapacity = 10000;
  int cellSpacing = 5;

  assert(cellSpacing > 0 && "cellSpacing must be > 0");

  size_t entityPoolSize = entityCapacity * sizeof(Entity);
  size_t spatialGridSize = entityCapacity * sizeof(SpatialEntry);
  size_t spatialGridCells =
      ((size_t)(100 / cellSpacing)) * ((size_t)(100 / cellSpacing));
  size_t spatialSparseSize = spatialGridCells * sizeof(size_t) * 2;
  size_t meshDataSize = 256 * 1024;

  size_t permanentArenaSize =
      entityPoolSize + spatialGridSize + spatialSparseSize + meshDataSize;
  size_t totalRequired = gameStateSize + permanentArenaSize;

  printf("=== Memory Allocation ===\n");
  printf("Entity Pool: %.2f MB (%zu entities)\n",
         entityPoolSize / (1024.0 * 1024.0), entityCapacity);
  printf("Spatial Grid Dense: %.2f MB\n", spatialGridSize / (1024.0 * 1024.0));
  printf("Spatial Grid Sparse: %.2f MB (%zu cells)\n",
         spatialSparseSize / (1024.0 * 1024.0), spatialGridCells);
  printf("Mesh Data: %.2f MB\n", meshDataSize / (1024.0 * 1024.0));
  printf("Total Permanent Arena: %.2f MB\n",
         permanentArenaSize / (1024.0 * 1024.0));
  printf("Total Required: %.2f MB\n", totalRequired / (1024.0 * 1024.0));
  printf("Total Available: %.2f MB\n",
         gameMemory->permanentMemorySize / (1024.0 * 1024.0));
  printf("========================\n");
  Assert(totalRequired <= gameMemory->permanentMemorySize);

  gameState->minBounds = (Vector3){0, 0, 0};
  gameState->maxBounds = (Vector3){100, 100, 0};

  void *permanentArenaBase =
      (char *)gameMemory->permamentMemory + gameStateSize;
  arena_init(&gameState->permanentArena, permanentArenaBase,
             permanentArenaSize);
  arena_init(&gameState->transientArena, gameMemory->transientMemory,
             gameMemory->transientMemorySize);
  gameState->permanentAllocator =
      create_arena_allocator(&gameState->permanentArena);
  gameState->transientAllocator =
      create_arena_allocator(&gameState->transientArena);

  arr_mat4 *transforms = arr_mat4_create(5, &gameState->permanentAllocator);

  // EntityPoolAllocator poolAllocator = {arena_alloc,
  // &gameState->permanentArena};
  //
  // gameState->instancedMesh =
  //     GenMeshCube(&gameState->permanentAllocator, 1.0f, 1.0f, 1.0f);
  // gameState->instancedMeshUpdated = true;
  //
  // gameState->entityPool = entityPool_InitInArena(poolAllocator,
  // entityCapacity);
  //
  // // SPATIAL GRID
  //
  // int numCellsX =
  //     (int)(gameState->maxBounds.x - gameState->minBounds.x) / cellSpacing;
  // int numCellsY =
  //     (int)(gameState->maxBounds.y - gameState->minBounds.y) / cellSpacing;
  //
  // gameState->sGrid = spatialGrid_create_with_dimensions(
  //     gameState->permanentAllocator, gameState->minBounds,
  //     gameState->maxBounds, cellSpacing, entityCapacity);
  //
  // for (int cy = 0; cy < numCellsY; cy++) {
  //   for (int cx = 0; cx < numCellsX; cx++) {
  //     Vector3 cellPos = {.x = gameState->minBounds.x + (cx * cellSpacing) +
  //                             (cellSpacing / 2.0f),
  //                        .y = gameState->minBounds.y + (cy * cellSpacing) +
  //                             (cellSpacing / 2.0f),
  //                        .z = 0.0f};
  //
  //     if (cx == 0 && cy == 0) {
  //       Entity testEntity =
  //           entity_create_physics_particle(cellPos, (Vector3){10, 10, 0});
  //       testEntity.c_transform.a = (Vector3){0, 0, 0}; // No acceleration
  //       testEntity.c_render.color =
  //           (Color){100, 100, 255, 200}; // Blue for test entities
  //       // entityPool_push(gameState->entityPool, testEntity);
  //       // entityPool_push(gameState->entityPool, testEntity);
  //       // entityPool_push(gameState->entityPool, testEntity);
  //       entityPool_push(gameState->entityPool, testEntity);
  //     }
  //     if (cx == 15 && cy == 15) {
  //       Entity testEntity =
  //           entity_create_physics_particle(cellPos, (Vector3){-10, -10, 0});
  //       testEntity.c_transform.a = (Vector3){0, 0, 0}; // No acceleration
  //       testEntity.c_render.color =
  //           (Color){100, 100, 255, 200}; // Blue for test entities
  //       // entityPool_push(gameState->entityPool, testEntity);
  //       entityPool_push(gameState->entityPool, testEntity);
  //     }
  //   }
  // }

  // Entity player =
  //     entity_create_physics_particle((Vector3){0, 0, 0}, (Vector3){0, 0,
  //     0});
  // player.followMouse = true;
  // entityPool_push(gameState->entityPool, player);
  // Entity spawner = entity_create_spawner_entity();
  // spawner.c_transform.pos = (Vector3){50, 50, 0};
  // entityPool_push(gameState->entityPool, spawner);
  gameMemory->isInitialized = true;
}
Entity entity_create_physics_particle(Vector3 pos, Vector3 velocity) {
  Entity e = {0};
  e.flags = ENTITY_FLAG_ACTIVE | ENTITY_FLAG_VISIBLE | ENTITY_FLAG_COLLIDING |
            ENTITY_FLAG_HAS_TRANSFORM | ENTITY_FLAG_HAS_RENDER |
            ENTITY_FLAG_HAS_COLLISION;
  e.c_transform = (c_Transform){.pos = {.x = pos.x, .y = pos.y, .z = 0},
                                .v = {.x = velocity.x, .y = velocity.y, .z = 0},
                                .a = {.x = 0, .y = 0, .z = 0},
                                .restitution = 0.90f};

  e.c_collision = (c_Collision){.radius = 1.0f,
                                .mass = 1.0f,
                                .inverseMass = 1.0f,
                                .collisionCount = 0,
                                .searchCount = 0};

  e.c_render = (c_Render){.renderRadius = 1.0f,
                          .color = {.r = 255, .b = 0, .g = 0, .a = 200}};

  // e.followMouse = true;
  return e;
}

Entity entity_create_spawner_entity(void) {
  Entity e = {0};
  e.flags = ENTITY_FLAG_HAS_SPAWNER | ENTITY_FLAG_ACTIVE |
            ENTITY_FLAG_HAS_TRANSFORM | ENTITY_FLAG_HAS_RENDER |
            ENTITY_FLAG_VISIBLE;

  e.spawnRate = 10.0f;
  e.clock = 0;
  // e.followMouse = true;
  e.c_render = (c_Render){.renderRadius = 1.5f,
                          .color = {.r = 0, .b = 255, .g = 0, .a = 200}};
  e.spawnCount = 2;
  e.c_transform = (c_Transform){.pos = {.x = 0, .y = 0, .z = 0},
                                .v = {.x = 5, .y = 5, .z = 0},
                                .a = {.x = 0, .y = 9.81, .z = 0},
                                .restitution = 0.90f};
  e.spawnEntity = malloc(sizeof(Entity));

  *e.spawnEntity =
      entity_create_physics_particle((Vector3){0, 0, 0}, (Vector3){0, 0, 0});
  e.spawnEntity->c_transform.a = (Vector3){0, 0, 0};
  return e;
}
