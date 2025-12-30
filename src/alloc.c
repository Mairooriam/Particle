#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#define Assert(Expression)                                                     \
  if (!(Expression)) {                                                         \
    *(int *)0 = 0;                                                             \
  }
#define KiloBytes(value) ((value) * 1024)
#define MegaBytes(value) ((KiloBytes(value)) * 1024)
#define GigaBytes(value) ((MegaBytes(value)) * 1024)
#define TeraBytes(value) ((GigaBytes(value)) * 1024)

typedef struct {
  bool isInitialized;
  void *permamentMemory;
  size_t permanentMemorySize;
  void *transientMemory;
  size_t transientMemorySize;
} GameMemory;

// RENDER COMPONENT
typedef struct {
  float r, g, b, a;
  float renderRadius; // Visual size (can differ from collision radius)
} c_Render;

// TRANSFORM COMPONENT
typedef struct {
  float pX, pY, pZ;
  float vX, vY, vZ;  // Velocity
  float aX, aY, aZ;  // acceleration
  float restitution; // 1 -> will bounce apart - 0 -> both will keep moving to
                     // same direction
} c_Transform;

typedef struct {
  float springConstat;
  float restLenght;
  size_t parent;
} c_Spring;

// COLLISION COMPONENT
typedef struct {
  float radius;
  float mass;
  float inverseMass;
  size_t collisionCount;
  size_t searchCount;
  float faX, faY, faZ; // force accum
} c_Collision;
typedef struct Entity Entity;
typedef struct Entity {
  size_t id;
  uint64_t flags;
  c_Transform c_transform;
  c_Render c_render;
  c_Collision c_collision;
  float spawnCount;
  float spawnRate;
  float clock;
  bool followMouse;
  c_Spring c_spring;
  Entity *next;
} Entity;
typedef struct Entities {
  Entity *items;
  size_t count;
  size_t capacity;
} Entities;
static void ConcatStrings(size_t sourceACount, char *sourceAstr,
                          size_t sourceBCount, char *sourceBstr,
                          size_t destCount, char *destStr) {
  (void)destCount;
  // TOOO: overflowing stuff
  for (size_t i = 0; i < sourceACount; i++) {
    *destStr++ = *sourceAstr++;
  }
  for (size_t i = 0; i < sourceBCount; i++) {
    *destStr++ = *sourceBstr++;
  }
  *destStr++ = 0;
}

typedef struct {
  size_t *entities_sparse;
  Entity *entities_dense; // Array of entities
  size_t capacity;        // Total capacity of the array
  size_t count;           // Number of active entities
  size_t *freeList_dense; // Stack of free indices
  size_t freeCount_dense; // Number of free slots
} EntityPool;

typedef struct {
  uint8_t *base;
  size_t size;
  size_t used;
} memory_arena;

#define memory_index                                                           \
  size_t // from casey experiment and see if this is good way to do this

void arena_init(memory_arena *arena, uint8_t *base, memory_index arena_size) {
  arena->base = base;
  arena->size = arena_size;
  arena->used = 0;
}

#define arena_PushStruct(Arena, type) (type *)_PushStruct(Arena, sizeof(type))
#define arena_PushStructs(Arena, type, count)                                  \
  _PushStruct(Arena, sizeof(type) * count)
// Ment for internal use do not call
// Zeroes out the pushed struct
static void *_PushStruct(memory_arena *arena, size_t size) {
  Assert(arena->used + size <= arena->size);
  void *result = arena->base + arena->used;
  arena->used += size;
  return result;
}

// Initialize the entity pool
EntityPool *EntityPoolInitInArena(memory_arena *arena, size_t capacity) {
  EntityPool *pool = arena_PushStruct(arena, EntityPool);
  pool->capacity = capacity;
  pool->entities_sparse = (size_t *)arena_PushStructs(arena, size_t, capacity);
  pool->entities_dense = (Entity *)arena_PushStructs(arena, Entity, capacity);
  pool->freeList_dense = (size_t *)arena_PushStructs(arena, size_t, capacity);
  pool->freeCount_dense = 0;

  for (size_t i = 0; i < capacity; i++) {
    pool->entities_sparse[i] = SIZE_MAX;
  }

  return pool;
}
void PrintEntityPoolMemoryLayout(EntityPool *pool) {
  printf("EntityPool Memory Layout:\n");
  printf("  Address of EntityPool: %p\n", (void *)pool);
  printf("  Sparse Array Address: %p (Size: %zu bytes)\n",
         (void *)pool->entities_sparse, pool->capacity * sizeof(size_t));
  printf("  Dense Array Address: %p (Size: %zu bytes)\n",
         (void *)pool->entities_dense, pool->capacity * sizeof(Entity));
  printf("  Free List Address: %p (Size: %zu bytes)\n",
         (void *)pool->freeList_dense, pool->capacity * sizeof(size_t));
  printf("  Capacity Address: %p (Value: %zu)\n", (void *)&pool->capacity,
         pool->capacity);
  printf("  Count Address: %p (Value: %zu)\n", (void *)&pool->count,
         pool->count);
  printf("  Free Count Address: %p (Value: %zu)\n",
         (void *)&pool->freeCount_dense, pool->freeCount_dense);
  printf("\n");
}
void EntityPoolPush(EntityPool *pool, Entity entity) {
  if (pool->freeCount_dense > 0) {
    // Reuse an entity from the free list
    size_t denseIndex = pool->freeList_dense[--pool->freeCount_dense];
    pool->entities_dense[denseIndex] = entity; // Copy the constructed entity
    pool->entities_sparse[entity.id] = denseIndex; // Update sparse mapping
  } else if (pool->count < pool->capacity) {
    // Allocate a new entity
    pool->entities_dense[pool->count] = entity; // Copy the constructed entity
    pool->entities_dense[pool->count].id = pool->count + 1; // Assign ID
    pool->entities_sparse[entity.id - 1] = pool->count; // Update sparse mapping
    pool->count++;
  } else {
    printf("Error: Entity capacity reached.\n");
  }
}

void EntityRemoveIdx(EntityPool *pool, size_t idx) {
  size_t denseIdx = pool->entities_sparse[idx - 1];
  Entity *e = &pool->entities_dense[denseIdx];
  if (!e) {
    printf("Tried to delete entity that doesn't exist\n");
    return;
  }
  pool->freeList_dense[pool->freeCount_dense++] =
      denseIdx;                              // Add dense index to free list
  pool->entities_sparse[idx - 1] = SIZE_MAX; // Reset sparse mapping
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
    printf("Sparse[%zu] = %zu\n", i, pool->entities_sparse[i]);
  }

  printf("\nDense Array (from %zu to %zu):\n", start, end);
  for (size_t i = start; i < end && i < pool->count; i++) {
    printf("Dense[%zu] = { ID: %zu, Position: (%f, %f) }\n", i,
           pool->entities_dense[i].id, pool->entities_dense[i].c_transform.pX,
           pool->entities_dense[i].c_transform.pY);
  }
  printf("\n");
}
typedef struct {
  EntityPool *entityPool;
  memory_arena entityArena;
} GameState;

int main(int argc, char const *argv[]) {
  LPVOID baseAddress = (LPVOID)TeraBytes((uint64_t)2);

  GameMemory gameMemory = {0};
  gameMemory.permanentMemorySize = MegaBytes(128);
  gameMemory.transientMemorySize = GigaBytes((uint64_t)1);

  uint64_t totalSize =
      gameMemory.permanentMemorySize + gameMemory.transientMemorySize;
  gameMemory.permamentMemory = VirtualAlloc(
      baseAddress, totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (!gameMemory.permamentMemory) {
    printf("Error: Memory allocation failed.\n");
    return -1;
  }
  gameMemory.transientMemory =
      ((uint8_t *)gameMemory.permamentMemory + gameMemory.permanentMemorySize);
  GameState *gamestate = (GameState *)gameMemory.permamentMemory;

  arena_init(&gamestate->entityArena,
             (uint8_t *)gameMemory.permamentMemory + sizeof(GameState),
             MegaBytes(64));

  // Initialize the entity pool
  gamestate->entityPool = EntityPoolInitInArena(&gamestate->entityArena, 1000);

  PrintEntityPoolMemoryLayout(gamestate->entityPool);
  // Allocate a large number of entities
  printf("Allocating 100 entities...\n");
  for (size_t i = 1; i <= 50; i++) {
    Entity entity = {0};
    entity.id = i;
    entity.c_transform.pX = i * 1.0f;
    entity.c_transform.pY = i * 2.0f;
    EntityPoolPush(gamestate->entityPool, entity);
  }

  // Print the first 20 entries of the sparse and dense arrays
  // PrintEntities(pool);
  PrintSparseAndDense(gamestate->entityPool, 0, 20);

  // Remove some entities
  printf("Removing entities with IDs 10, 20, 30, 40, 50...\n");
  EntityRemoveIdx(gamestate->entityPool, 5);
  EntityRemoveIdx(gamestate->entityPool, 10);
  EntityRemoveIdx(gamestate->entityPool, 15);
  EntityRemoveIdx(gamestate->entityPool, 20);
  EntityRemoveIdx(gamestate->entityPool, 25);

  // Print the first 20 entries of the sparse and dense arrays after
  // PrintEntities(gamestate->entityPool);
  PrintSparseAndDense(gamestate->entityPool, 0, 20);

  // Allocate new entities to reuse the removed slots
  printf("Allocating 5 new entities...\n");
  for (size_t i = 101; i <= 105; i++) {
    Entity entity = {0};
    entity.id = i;
    entity.c_transform.pX = i * 1.0f;
    entity.c_transform.pY = i * 2.0f;
    EntityPoolPush(gamestate->entityPool, entity);
  }

  // Print the first 20 entries of the sparse and dense arrays after
  // reallocation
  // PrintEntities(gamestate->entityPool);
  PrintSparseAndDense(gamestate->entityPool, 0, 20);

  // Allocate more entities to fill the gamestate->entityPool
  printf("Allocating 895 more entities to fill the gamestate->entityPool...\n");
  for (size_t i = 106; i <= 1000; i++) {
    Entity entity = {0};
    entity.id = i;
    entity.c_transform.pX = i * 1.0f;
    entity.c_transform.pY = i * 2.0f;
    EntityPoolPush(gamestate->entityPool, entity);
  }

  // Print the last 20 entries of the sparse and dense arrays
  // PrintEntities(gamestate->entityPool);
  PrintSparseAndDense(gamestate->entityPool, 95, 110);
  PrintEntityPoolMemoryLayout(gamestate->entityPool);
  return 0;
}
