// application.c
#include "application.h"
#include "shared.h"
#include "stdio.h"
// if moving to c++ to prevent name mangling
// extern "C" GAME_UPDATE(game_update) { pos->y++; }
GAME_UPDATE(game_update) {
  Assert(sizeof(GameState) <= gameMemory->permanentMemorySize);

  GameState *gameState = (GameState *)gameMemory->permamentMemory;
  if (!gameMemory->isInitialized) {
    size_t gameStateSize = sizeof(GameState);
    size_t permanentArenaSize = 32 * 1024 * 1024;
    gameState->minBounds = (Vector3){0, 0, 0};
    gameState->maxBounds = (Vector3){100, 100, 0};
    // Reserve space for arena, then calculate maxEntities from remaining
    size_t availableForEntities =
        gameMemory->permanentMemorySize - gameStateSize - permanentArenaSize;
    size_t maxEntities = availableForEntities / sizeof(Entity);

    // Initialize arenas
    size_t permanentArenaOffset = gameStateSize + maxEntities * sizeof(Entity);
    void *permanentArenaBase =
        (char *)gameMemory->permamentMemory + permanentArenaOffset;
    Assert(permanentArenaSize > 0);

    arena_init(&gameState->permanentArena, permanentArenaBase,
               permanentArenaSize);
    arena_init(&gameState->transientArena, gameMemory->transientMemory,
               gameMemory->transientMemorySize);
    size_t entityCapacity = 10000;
    // Create allocators for EntityPool, mesh, and spatial grid
    EntityPoolAllocator poolAllocator = {arena_alloc, &gameState->permanentArena};
    MemoryAllocator meshAllocator = create_arena_allocator(&gameState->permanentArena);
    MemoryAllocator spatialAllocator = create_arena_allocator(&gameState->permanentArena);

    // Mesh generation
    gameState->instancedMesh = GenMeshCube(meshAllocator, 1.0f, 1.0f, 1.0f);
    gameState->instancedMeshUpdated = true;

    // ENTITY POOL
    gameState->entityPool = entityPool_InitInArena(poolAllocator, entityCapacity);

    // SPATIAL GRID
    gameState->sGrid = spatialGrid_create_with_dimensions(spatialAllocator, gameState->minBounds, gameState->maxBounds, 25, entityCapacity);

    Entity player = entity_create_physics_particle((Vector3){0, 0, 0}, (Vector3){0, 0, 0});
    player.followMouse = true;
    entityPool_push(gameState->entityPool, player);
    Entity spawner = entity_create_spawner_entity();
    entityPool_push(gameState->entityPool, spawner);
    gameMemory->isInitialized = true;
  }

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

  if (is_key_released(input, &gameState->lastFrameInput, KEY_SPACE)) {
    // PrintSparseAndDense(gameState->entityPool, 0, 50);
    printf("Space released!\n");
  }

  if (is_key_down(input, KEY_SPACE)) {
    printf("Space is down!\n");
  }

  EntityPool *entityPool = gameState->entityPool;

  // Update entities
  for (size_t i = 0; i < entityPool->entities_dense.count; i++) {
    Entity *e = &entityPool->entities_dense.items[i];
    if (e->flags & ENTITY_FLAG_ACTIVE) {
      if (e->flags & ENTITY_FLAG_HAS_TRANSFORM) {
        e->c_transform.a = (Vector3){0, -9.81, 0};
        update_entity_position(e, frameTime, input->mousePos);
        update_entity_boundaries(e, gameState->maxBounds.x,
                                 gameState->minBounds.x, gameState->maxBounds.y,
                                 gameState->minBounds.y, gameState->maxBounds.z,
                                 gameState->minBounds.z);
      }
      if (e->flags & ENTITY_FLAG_HAS_SPAWNER) {
        // e->c_transform.v = (Vector3){0, 0, 0};
        e->c_transform.v =
            Vector3Add(e->c_transform.v, (Vector3){1, 2.0f, 0.5f});
        update_spawners(frameTime, e, entityPool);
      }
    }
  }
  update_spatial(gameState->sGrid, &gameState->entityPool->entities_dense);
  size_t cell = 0; // or any cell index you want to inspect
  size_t start = gameState->sGrid->spatialSparse.items[cell];
  size_t end = (cell + 1 < gameState->sGrid->spatialSparse.capacity)
                   ? gameState->sGrid->spatialSparse.items[cell + 1]
                   : gameState->sGrid->spatialDense.count;
  printf("Entities in cell %zu: ", cell);
  for (size_t i = start; i < end; i++) {
    printf("%zu ", gameState->sGrid->spatialDense.items[i]);
  }
  printf("\n");

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

Entity entity_create_physics_particle(Vector3 pos, Vector3 velocity) {
  Entity e = {0};
  e.flags = ENTITY_FLAG_ACTIVE | ENTITY_FLAG_VISIBLE | ENTITY_FLAG_COLLIDING |
            ENTITY_FLAG_HAS_TRANSFORM | ENTITY_FLAG_HAS_RENDER |
            ENTITY_FLAG_HAS_COLLISION;
  e.c_transform = (c_Transform){.pos = {.x = pos.x, .y = pos.y, .z = 0},
                                .v = {.x = velocity.x, .y = velocity.y, .z = 0},
                                .a = {.x = 0, .y = 0, .z = 0},
                                .restitution = 0.90f};

  e.c_collision = (c_Collision){.radius = 25.0f,
                                .mass = 1.0f,
                                .inverseMass = 1.0f,
                                .collisionCount = 0,
                                .searchCount = 0};

  e.c_render = (c_Render){.renderRadius = 24.0f,
                          .color = {.r = 255, .b = 0, .g = 0, .a = 200}};

  // e.followMouse = true;
  return e;
}

Entity entity_create_spawner_entity() {
  Entity e = {0};
  e.flags = ENTITY_FLAG_HAS_SPAWNER | ENTITY_FLAG_ACTIVE |
            ENTITY_FLAG_HAS_TRANSFORM | ENTITY_FLAG_HAS_RENDER |
            ENTITY_FLAG_VISIBLE;

  e.spawnRate = 1.0f;
  e.clock = 0;
  // e.followMouse = true;
  e.c_render = (c_Render){.renderRadius = 24.0f,
                          .color = {.r = 0, .b = 255, .g = 0, .a = 200}};
  e.spawnCount = 1;
  e.c_transform = (c_Transform){.pos = {.x = 0, .y = 0, .z = 0},
                                .v = {.x = 50, .y = 50, .z = 50},
                                .a = {.x = 0, .y = 9.81, .z = 0},
                                .restitution = 0.90f};
  e.spawnEntity = malloc(sizeof(Entity));

  *e.spawnEntity =
      entity_create_physics_particle((Vector3){0, 0, 0}, (Vector3){0, 0, 0});
  e.spawnEntity->c_transform.a = (Vector3){0, 0, 0};
  return e;
}
