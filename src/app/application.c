// application.c
#include "application.h"
#include "app/application_types.h"
#include "entityPool_types.h"
#include "entity_types.h"
#include "shared.h"
#include "stdio.h"
// if moving to c++ to prevent name mangling
// extern "C" GAME_UPDATE(game_update) { pos->y++; }
GAME_UPDATE(game_update) {
  Assert(sizeof(GameState) <= gameMemory->permanentMemorySize);

  // ==================== INITIALIZATION ====================
  GameState *gameState = (GameState *)gameMemory->permamentMemory;
  if (!gameMemory->isInitialized) {
    handle_init(gameMemory, gameState);
  }
  if (gameMemory->reloadDLLHappened) {
    gameState->permanentAllocator.alloc = arena_alloc;
    gameState->permanentAllocator.free = arena_free;
    gameState->transientAllocator.alloc = arena_alloc;
    gameState->transientAllocator.free = arena_free;
    gameMemory->reloadDLLHappened = false;
  }

  arena_reset(&gameState->transientArena);

  handle_input(gameState, input);

  handle_update(gameState, frameTime, input);

  // ==================== DEBUG STUFF ====================
  // size_t cell = 0; // or any cell index you want to inspect
  // size_t start = gameState->sGrid->spatialSparse.items[cell];
  // size_t end = (cell + 1 < gameState->sGrid->spatialSparse.capacity)
  //                  ? gameState->sGrid->spatialSparse.items[cell + 1]
  //                  : gameState->sGrid->spatialDense.count;
  // printf("Entities in cell %zu: ", cell);
  // for (size_t i = start; i < end; i++) {
  //   printf("%zu ", gameState->sGrid->spatialDense.items[i].entityId);
  // }
  // printf("\n");

  // ==================== PUSING RENDER COMMANDS ETC ====================
  render(gameMemory, gameState);

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

  EntityPoolAllocator poolAllocator = {arena_alloc, &gameState->permanentArena};

  gameState->instancedMesh =
      GenMeshCube(&gameState->permanentAllocator, 1.0f, 1.0f, 1.0f);
  gameState->instancedMeshUpdated = true;

  gameState->entityPool = entityPool_InitInArena(poolAllocator, entityCapacity);

  // SPATIAL GRID

  int numCellsX =
      (int)(gameState->maxBounds.x - gameState->minBounds.x) / cellSpacing;
  int numCellsY =
      (int)(gameState->maxBounds.y - gameState->minBounds.y) / cellSpacing;

  gameState->sGrid = spatialGrid_create_with_dimensions(
      gameState->permanentAllocator, gameState->minBounds, gameState->maxBounds,
      cellSpacing, entityCapacity);

  for (int cy = 0; cy < numCellsY; cy++) {
    for (int cx = 0; cx < numCellsX; cx++) {
      Vector3 cellPos = {.x = gameState->minBounds.x + (cx * cellSpacing) +
                              (cellSpacing / 2.0f),
                         .y = gameState->minBounds.y + (cy * cellSpacing) +
                              (cellSpacing / 2.0f),
                         .z = 0.0f};

      if (cx == 0 && cy == 0) {
        Entity testEntity =
            entity_create_physics_particle(cellPos, (Vector3){10, 10, 0});
        testEntity.c_transform.a = (Vector3){0, 0, 0}; // No acceleration
        testEntity.c_render.color =
            (Color){100, 100, 255, 200}; // Blue for test entities
        // entityPool_push(gameState->entityPool, testEntity);
        // entityPool_push(gameState->entityPool, testEntity);
        // entityPool_push(gameState->entityPool, testEntity);
        entityPool_push(gameState->entityPool, testEntity);
      }
      if (cx == 15 && cy == 15) {
        Entity testEntity =
            entity_create_physics_particle(cellPos, (Vector3){-10, -10, 0});
        testEntity.c_transform.a = (Vector3){0, 0, 0}; // No acceleration
        testEntity.c_render.color =
            (Color){100, 100, 255, 200}; // Blue for test entities
        // entityPool_push(gameState->entityPool, testEntity);
        entityPool_push(gameState->entityPool, testEntity);
      }
    }
  }

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
