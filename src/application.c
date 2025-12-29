// application.c
#include "application.h"
#include "stdio.h"
// if moving to c++ to prevent name mangling
// extern "C" GAME_UPDATE(game_update) { pos->y++; }
GAME_UPDATE(game_update) {
  Assert(sizeof(GameState) <= gameMemory->permanentMemorySize);
  // float lerpFactor = 0.5f;
  // static float posZ = 1;
  // if (posZ > 10) {
  //   posZ = 1;
  // } else {
  //   posZ += 10;
  // }
  // input->camera.position.z = Lerp(1, posZ, lerpFactor);
  // // cTp1->pos.y = Lerp(cTp1->pos.y, mouseWorldPos.y, lerpFactor);
  // // cTp1->pos.z = 0.0f;
  // // input->camera.position.z = 200;
  // input->camera.target.x = 400;
  // input->camera.target.y = 200;

  GameState *gameState = (GameState *)gameMemory->permamentMemory;
  if (!gameMemory->isInitialized) {
    size_t gameStateSize = sizeof(GameState);
    size_t permanentArenaSize = 32 * 1024 * 1024;

    // Reserve space for arena, then calculate maxEntities from remaining
    size_t availableForEntities =
        gameMemory->permanentMemorySize - gameStateSize - permanentArenaSize;
    size_t maxEntities = availableForEntities / sizeof(Entity);

    Entity *entitiesBuffer =
        (Entity *)((char *)gameMemory->permamentMemory + gameStateSize);
    Entities_init_with_buffer(&gameState->entities, maxEntities,
                              entitiesBuffer);

    // Initialize arenas
    size_t permanentArenaOffset = gameStateSize + maxEntities * sizeof(Entity);
    void *permanentArenaBase =
        (char *)gameMemory->permamentMemory + permanentArenaOffset;
    Assert(permanentArenaSize > 0);

    gameState->permanentArena =
        initialize_arena(permanentArenaBase, permanentArenaSize);
    gameState->transientArena = initialize_arena(
        gameMemory->transientMemory, gameMemory->transientMemorySize);

    // Mesh generation
    gameState->instancedMesh =
        GenMeshCube(&gameState->permanentArena, 1.0f, 1.0f, 1.0f);
    gameState->instancedMeshUpdated = true;

    Entity player = entity_create_physics_particle((Vector3){400, 300, 200},
                                                   (Vector3){0, 9.81, 0});
    player.followMouse = true;
    entity_add(&gameState->entities, player);

    Entity spawner = entity_create_spawner_entity();
    entity_add(&gameState->entities, spawner);

    gameMemory->isInitialized = true;
  }

  gameState->mouseDelta =
      Vector2Subtract(input->mousePos, gameState->lastFrameInput.mousePos);

  if (is_key_pressed(input, &gameState->lastFrameInput, KEY_SPACE)) {
    printf("Space pressed!\n");
  }

  if (is_key_released(input, &gameState->lastFrameInput, KEY_SPACE)) {
    printf("Space released!\n");
  }

  if (is_key_down(input, KEY_SPACE)) {
    printf("Space is down!\n");
  }

  Entities *entities = &gameState->entities;

  // Update entities
  for (size_t i = 0; i < entities->count; i++) {
    Entity *e = &entities->items[i];
    if (e->flags & ENTITY_FLAG_ACTIVE) {
      if (e->flags & ENTITY_FLAG_HAS_TRANSFORM) {
        e->c_transform.a = (Vector3){0, 9.81, 0};
        update_entity_position(e, frameTime, input->mousePos);
        update_entity_boundaries(e, 800, 0, 600, 0, 100,
                                 -100); // Added Z bounds
      }
      if (e->flags & ENTITY_FLAG_HAS_SPAWNER) {
        update_spawners(frameTime, e, entities);
      }
    }
  }

  // Render entities (instanced)
  RenderQueue *renderQueue = (RenderQueue *)gameMemory->transientMemory;
  renderQueue->count = 0;

  // Allocate transforms in transient memory
  Matrix *sphereTransforms =
      (Matrix *)((char *)gameMemory->transientMemory + sizeof(RenderQueue));
  size_t maxTransforms =
      (gameMemory->transientMemorySize - sizeof(RenderQueue)) / sizeof(Matrix);
  size_t sphereCount = 0;

  for (size_t i = 0; i < entities->count && sphereCount < maxTransforms; i++) {
    Entity *e = &entities->items[i];
    if ((e->flags & ENTITY_FLAG_VISIBLE) &&
        (e->flags & ENTITY_FLAG_HAS_RENDER)) {
      // Collect transform for instanced drawing
      sphereTransforms[sphereCount++] = MatrixTranslate(
          e->c_transform.pos.x, e->c_transform.pos.y, e->c_transform.pos.z);
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

  gameState->lastFrameInput = *input;
}

void update_spawners(float frameTime, Entity *e, Entities *entities) {
  e->clock += frameTime;
  if (e->clock > (1 / e->spawnRate)) {
    for (size_t i = 0; i < e->spawnCount; i++) {
      e->spawnEntity->c_transform.pos = e->c_transform.pos;
      entity_add(entities, *e->spawnEntity);
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
void entity_add(Entities *entities, Entity entity) {
  if (!entities) {
    fprintf(stderr, "Entities pointer is NULL\n");
    return;
  }
  if (entities->count >= entities->capacity) {
    fprintf(stderr, "Entities buffer is full, cannot add more entities\n");
    return;
  }
  entity.id = entities->count;
  entities->items[entities->count++] = entity;
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
  e.flags =
      ENTITY_FLAG_HAS_SPAWNER | ENTITY_FLAG_ACTIVE | ENTITY_FLAG_HAS_TRANSFORM;

  e.spawnRate = 20.0f;
  e.clock = 0;
  // e.followMouse = true;
  e.c_render = (c_Render){.renderRadius = 24.0f,
                          .color = {.r = 0, .b = 255, .g = 0, .a = 200}};
  e.spawnCount = 5;
  e.c_transform = (c_Transform){.pos = {.x = 0, .y = 0, .z = 0},
                                .v = {.x = 0, .y = 0, .z = 0},
                                .a = {.x = 0, .y = 0, .z = 0},
                                .restitution = 0.90f};
  e.spawnEntity = malloc(sizeof(Entity));

  *e.spawnEntity = entity_create_physics_particle((Vector3){0, 0, 0},
                                                  (Vector3){100, 100, 0});
  e.spawnEntity->c_transform.a = (Vector3){0, 9.81, 0};
  return e;
}
