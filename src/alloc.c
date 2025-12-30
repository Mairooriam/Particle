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
  bool active;
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
  char *base;
  size_t size;
  size_t used;
} memory_arena;

static void *_PushSizeZero(memory_arena *arena, size_t size) {
  Assert(arena->used + size <= arena->size);
  void *result = arena->base + arena->used;
  arena->used += size;
  memset(result, 0, size);
  return result;
}

// Initialize the entity pool
EntityPool EntityPoolInit(memory_arena *arena, size_t capacity) {
  EntityPool pool = {0};
  pool.entities_sparse =
      (size_t *)_PushSizeZero(arena, sizeof(size_t) * capacity);
  pool.entities_dense =
      (Entity *)_PushSizeZero(arena, sizeof(Entity) * capacity);
  pool.capacity = capacity;
  pool.freeList_dense =
      (size_t *)_PushSizeZero(arena, sizeof(size_t) * capacity);
  pool.freeCount_dense = 0;
  return pool;
}

void EntityAlloc(EntityPool *pool, Entity entity) {
  if (pool->freeCount_dense > 0) {
    // Reuse an entity from the free list
    size_t index = pool->freeList_dense[--pool->freeCount_dense];
    pool->entities_dense[index] = entity; // Copy the constructed entity
    pool->entities_dense[index].active = true;
    pool->entities_sparse[entity.id - 1] = index; // Update sparse mapping
  } else if (pool->count < pool->capacity) {
    // Allocate a new entity
    pool->entities_dense[pool->count] = entity; // Copy the constructed entity
    pool->entities_dense[pool->count].id = pool->count + 1; // Assign ID
    pool->entities_dense[pool->count].active = true;
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
    printf("Tried to delete entity that doesn't exist");
    return;
  }
  if (e->active) {
    e->active = false;
    pool->freeList_dense[pool->freeCount_dense++] = e->id;
  }
}
void PrintEntities(EntityPool *pool) {
  printf("Active Entities:\n");
  for (size_t i = 0; i < pool->count; i++) {
    if (pool->entities_dense[i].active) {
      printf("Entity ID: %zu\n", pool->entities_dense[i].id);
    }
  }
}

memory_arena arena_init(void *buffer, size_t arena_size) {
  memory_arena arena = {0};
  arena.base = (char *)buffer;
  arena.size = arena_size;
  arena.used = 0;
  return arena;
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
    printf("Dense[%zu] = { ID: %zu, Active: %s, Position: (%f, %f) }\n",
           i,
           pool->entities_dense[i].id,
           pool->entities_dense[i].active ? "true" : "false",
           pool->entities_dense[i].c_transform.pX,
           pool->entities_dense[i].c_transform.pY);
  }
  printf("\n");
}
int main(int argc, char const *argv[]) {
  LPVOID baseAddress = (LPVOID)TeraBytes((uint64_t)2);

  GameMemory gameMemory = {0};
  gameMemory.permanentMemorySize = MegaBytes(64);
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

  memory_arena arena = arena_init(gameMemory.permamentMemory, MegaBytes(64));

  // Initialize the entity pool
  EntityPool pool = EntityPoolInit(&arena, 1000);

  // Construct and allocate entities
  Entity entity1 = {0};
  entity1.id = 1;
  entity1.c_transform.pX = 10.0f;
  entity1.c_transform.pY = 20.0f;
  EntityAlloc(&pool, entity1);

  Entity entity2 = {0};
  entity2.id = 2;
  entity2.c_transform.pX = 30.0f;
  entity2.c_transform.pY = 40.0f;
  EntityAlloc(&pool, entity2);

  Entity entity3 = {0};
  entity3.id = 3;
  entity3.c_transform.pX = 50.0f;
  entity3.c_transform.pY = 60.0f;
  EntityAlloc(&pool, entity3);

  PrintEntities(&pool);
  PrintSparseAndDense(&pool, 0, 10); // Print the first 10 entries

  // Remove an entity
  printf("Remove index 2\n");
  EntityRemoveIdx(&pool, 2);

  PrintEntities(&pool);
  PrintSparseAndDense(&pool, 0, 10); // Print the first 10 entries

  // Add a new entity (should reuse the removed entity)
  printf("Allocating index 4\n");
  Entity entity4 = {0};
  entity4.id = 4;
  entity4.c_transform.pX = 70.0f;
  entity4.c_transform.pY = 80.0f;
  EntityAlloc(&pool, entity4);

  PrintEntities(&pool);
  PrintSparseAndDense(&pool, 0, 10); // Print the first 10 entries

  return 0;
}
