#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
// https://ajmmertens.medium.com/doing-a-lot-with-a-little-ecs-identifiers-25a72bd2647
// https://doc.lagout.org/security/Hackers%20Delight.pdf
// https://stackoverflow.com/questions/69023477/what-are-some-good-resources-for-learning-bit-shifting
// https://graphics.stanford.edu/%7Eseander/bithacks.html
//
//
#define memory_index                                                           \
  size_t // from casey experiment and see if this is good way to do this

typedef struct {
  float x, y;
} Vector2;

typedef struct {
  float x, y, z;
} Vector3;

typedef struct Entity Entity;

#define arr_push(arr, element)                                                 \
  do {                                                                         \
    if ((arr)->count >= (arr)->capacity) {                                     \
      /*No capacity fix your for loop*/                                        \
      assert(0 && "Fix you loop");                                             \
    }                                                                          \
    (arr)->items[(arr)->count] = (element);                                    \
    (arr)->count++;                                                            \
  } while (0)
#define arr_insert(arr, idx, element)                                          \
  do {                                                                         \
    if ((idx) >= (arr)->capacity) {                                            \
      /*No capacity fix your for loop*/                                        \
      assert(0 && "Fix your loop");                                            \
    }                                                                          \
    (arr)->items[(idx)] = (element);                                           \
    (arr)->count++;                                                            \
  } while (0)

// ==================== ENTITY POOL ====================
#define entityID size_t
typedef struct {
  entityID *items;
  size_t count;
  size_t capacity;
} arr_entityID;

typedef struct {
  Vector3 pos;
  Vector3 v;           // Velocity
  Vector3 a;           // acceleration
  float restitution;   // 1 -> will bounce apart - 0 -> both will keep moving to
                       // same direction
  Vector3 previousPos; // https://gafferongames.com/post/fix_your_timestep/
                       // currently not being used
  Vector3 lastGridPos; // last position used for spatial grid
  bool needsGridUpdate; // flag if entity moved enough to need update
} c_Transform;
struct Entity {
  entityID id;
};

typedef struct {
  Entity *items;
  size_t count;
  size_t capacity;
} arr_Entity;

typedef struct Entities {
  Entity *items;
  size_t count;
  size_t capacity;
} Entities;

typedef struct {
  arr_entityID entities_sparse; // stores indicies for entities located in dense
  arr_Entity entities_dense;    // contigious entity arr
  size_t capacity; // Total capacity of pool. other arrays share the same size
  arr_entityID freeIds;
  size_t nextId; // Next unique ID
} EntityPool;

// ==================== ARENA ====================
typedef struct {
  unsigned char *base;
  size_t size;
  size_t used;
} memory_arena;

static void arena_init(memory_arena *arena, unsigned char *base,
                       memory_index arena_size) {
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
  assert(arena->used + size <= arena->size);
  void *result = arena->base + arena->used;
  arena->used += size;
  return result;
}

// Initialize the entity pool
static EntityPool *entityPool_InitInArena(memory_arena *arena,
                                          size_t capacity) {
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
static void entityPool_clear(EntityPool *ePool) {
  // TODO: memset the items to something?
  ePool->entities_dense.count = 0;
  ePool->entities_sparse.count = 0;
  ePool->freeIds.count = 0;
  ePool->nextId = 1;
}
static void PrintSparseAndDense(EntityPool *pool, size_t start, size_t end) {
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
static size_t entityPool_GetNextId(EntityPool *pool) {
  if (pool->freeIds.count > 0) {
    return pool->freeIds.items[--pool->freeIds.count];
  } else {
    return pool->nextId++; // return new id
  }
}

static void EntityPoolPush(EntityPool *pool, Entity entity) {}

static void EntityPoolRemoveIdx(EntityPool *pool, size_t idx) {}

int main(int argc, char *argv[]) {

  memory_arena arena = {0};
  size_t arenaSize = 1024 * 10;
  uint8_t *base = (uint8_t *)malloc(arenaSize);
  arena_init(&arena, base, arenaSize);

  EntityPool *ePool = entityPool_InitInArena(&arena, 100);

  for (size_t i = 0; i < ePool->entities_dense.capacity; i++) {
    arr_insert(&ePool->entities_dense, i, (Entity){.id = i + 1});
  }
  for (size_t i = 0; i < ePool->entities_sparse.capacity; i++) {
    arr_insert(&ePool->entities_sparse, i, i + 1);
  }
  for (size_t i = 0; i < ePool->freeIds.capacity; i++) {
    arr_insert(&ePool->freeIds, i, i + 2);
  }
  ePool->nextId = 10;

  entityPool_clear(ePool);

  size_t newid = entityPool_GetNextId(ePool);

  printf("hello world");
  return 0;
}
