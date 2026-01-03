#include <stdbool.h>
#include <stddef.h>
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

#define arr_remove_swap(arr, idx)                                              \
  do {                                                                         \
    if ((idx) < (arr)->count) {                                                \
      (arr)->items[(idx)] = (arr)->items[(arr)->count - 1];                    \
      (arr)->count--;                                                          \
    }                                                                          \
  } while (0)

#ifdef SLOW_CODE_ALLOWED
#define arr_get_safe(arr, idx)                                                 \
  (assert((idx) < (arr)->count && "Index out of bounds"), (arr)->items[(idx)])
#else
#define arr_get_safe(arr, idx) ((arr)->items[(idx)])
#endif
// ==================== ENTITY POOL ====================
#define en_identifier uint64_t
#define en_id uint32_t
#define en_gen uint16_t
typedef struct {
  en_id *items;
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
  en_identifier identifier;
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
  memset(result, 0, size);
  return result;
}

// Initialize the entity pool
static EntityPool *entityPool_InitInArena(memory_arena *arena,
                                          size_t capacity) {
  EntityPool *pool = arena_PushStruct(arena, EntityPool);
  pool->capacity = capacity;

  pool->entities_sparse.items = (en_id *)arena_PushStructs(
      arena, en_id, capacity + 1); // +1 for unused index 0
  pool->entities_sparse.capacity = capacity + 1;
  pool->entities_sparse.count = 0;

  pool->entities_dense.items =
      (Entity *)arena_PushStructs(arena, Entity, capacity);
  pool->entities_dense.capacity = capacity;
  pool->entities_dense.count = 0;

  pool->freeIds.items =
      (en_id *)arena_PushStructs(arena, en_id, capacity); // Free ID stack
  pool->freeIds.capacity = capacity;
  pool->freeIds.count = 0;

  pool->nextId = 1; // Start at 1

  return pool;
}
static void entityPool_clear(EntityPool *ePool) {
  ePool->entities_dense.count = 0;
  ePool->entities_sparse.count = 0;
  ePool->freeIds.count = 0;
  ePool->nextId = 1;

  memset(ePool->entities_dense.items, 0,
         ePool->entities_dense.capacity * sizeof(Entity));

  memset(ePool->entities_sparse.items, 0,
         ePool->entities_sparse.capacity * sizeof(en_id));

  memset(ePool->freeIds.items, 0, ePool->freeIds.capacity * sizeof(en_id));
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
    printf("Sparse[%zu] = %i\n", i, pool->entities_sparse.items[i]);
  }

  printf("\nDense Array (from %zu to %zu):\n", start, end);
  for (size_t i = start; i < end && i < pool->entities_dense.count; i++) {
    printf("Dense[%zu] = { ID: %zu }\n", i,
           pool->entities_dense.items[i].identifier);
  }
  printf("\n");
}

// internal function do not use. modifies the pool
static en_id _entityPool_GetNextId(EntityPool *pool) {
  if (pool->freeIds.count > 0) {
    return pool->freeIds.items[--pool->freeIds.count];
  } else {
    return pool->nextId++; // return new id
  }
}

static en_id _entityPool_PeekNextId(EntityPool *pool) {
  if (pool->freeIds.count > 0) {
    return pool->freeIds.items[pool->freeIds.count - 1];
  } else {
    return pool->nextId;
  }
}
static void strrev_impl(char *head) {
  if (!head)
    return;
  char *tail = head;
  while (*tail)
    ++tail; // find the 0 terminator, like head+strlen
  --tail;   // tail points to the last real char
            // head still points to the first
  for (; head < tail; ++head, --tail) {
    // walk pointers inwards until they meet or cross in the middle
    char h = *head, t = *tail;
    *head = t; // swapping as we go
    *tail = h;
  }
}

// straight from AI
#define NUM_TO_BINARY_STR(buf, size, num)                                      \
  do {                                                                         \
    size_t pos = 0;                                                            \
    size_t total_bits = sizeof(num) * 8;                                       \
    for (size_t i = 0; i < total_bits; i++) {                                  \
      size_t shift_amount = total_bits - 1 - i;                                \
      uint64_t mask = (shift_amount < 64) ? (1ULL << shift_amount) : 0;        \
      uint64_t masked = ((uint64_t)(num) & mask);                              \
      char c = masked ? '1' : '0';                                             \
      snprintf((buf) + pos, (size) - pos, "%c", c);                            \
      pos++;                                                                   \
      if ((i + 1) % 8 == 0 && (i + 1) < total_bits) {                          \
        snprintf((buf) + pos, (size) - pos, " ");                              \
        pos++;                                                                 \
      }                                                                        \
    }                                                                          \
    (buf)[pos] = '\0';                                                         \
  } while (0)

//   255|8                           65k                 16 million
// |00000000|00000000 00000000|00000000 00000000|00000000 00000000 00000000|
// |  flags |    spare        |   generation    |         ID               |
// TODO: make these into #defines?
uint64_t en_mask_id = 0xFFFFFF; // 24 bits for raw id -> 16 million entities;
uint64_t en_mask_generation = 0xFFFF; // 16 bits for generation
uint64_t en_mask_generation_offset = 24;
uint64_t en_mask_spare = 0xFFFF;
uint64_t en_mask_spare_offset = 40;
uint64_t en_mask_flags = 0xFF;
uint64_t en_mask_flags_offset = 56;

// TODO: should parameters be bigger? if i change layout?
static en_identifier _entity_id_create(uint32_t _idx, uint16_t _gen,
                                       uint16_t _spare, uint8_t _flags) {
  en_identifier idx = 0;
  idx = en_mask_id & _idx;
  idx |= (en_mask_generation & _gen) << en_mask_generation_offset;
  idx |= (en_mask_spare & _spare) << en_mask_spare_offset;
  idx |= (en_mask_flags & _flags) << en_mask_flags_offset;
  return idx;
}

// TODO: should i add overflow protection? 0xFF++ -> 0x00
// TODO: have it return it or edit the value?
// TODO: helpers that use this function? pass num++ as func? ->
// modify_number_at_offset(num, offset, mask, func)
static en_identifier _increment_number_at_offset(en_identifier idx,
                                                 size_t offset, size_t mask) {
  size_t num = 0;
  num |= ((idx) >> offset) & mask;
  num++;
  idx = idx & ~(mask << offset);
  idx |= ((num) << offset);
  return idx;
}

static en_identifier _get_number_at_offset(en_identifier idx, size_t offset,
                                           size_t mask) {
  return (idx >> offset) & mask;
}

static en_id en_get_id(en_identifier idx) {
  return _get_number_at_offset(idx, 0, en_mask_id);
}

static en_gen en_get_generationid(en_identifier identifier) {
  return _get_number_at_offset(identifier, en_mask_generation_offset,
                               en_mask_generation);
}
static void _set_number_at_offset(size_t value, size_t *num, size_t offset,
                                  size_t mask) {
  *num = *num & ~(mask << offset);
  *num |= ((value) << offset);
}
static void en_set_identifier_id(en_identifier *identifier, en_id value) {
  _set_number_at_offset(value, identifier, 0, en_mask_id);
}

static Entity *entityPool_from_dense_get_entity(EntityPool *pool, en_id idx) {
  return &pool->entities_dense.items[pool->entities_sparse.items[idx]];
}
static en_identifier entityPool_from_dense_get_identifier(EntityPool *pool,
                                                          en_id idx) {
  return pool->entities_dense.items[pool->entities_sparse.items[idx]]
      .identifier;
}
static void entityPool_insert(EntityPool *pool, Entity entity) {
  en_id newID = en_get_id(entity.identifier);

  if (newID == 0) {
    assert(0 && "tried to insert entity with id:0. which is not allowed. Ids "
                "start from 1");
  }

  // if id that is being inserted skips new ids need to add free ids
  // TODO: very bad scaling code. improve if end up inserting alot
  // TODO: this is terrible
  if (newID >= pool->nextId) {
    for (en_id skipped = pool->nextId; skipped < newID; skipped++) {
      if (pool->freeIds.count < pool->freeIds.capacity) {
        arr_push(&pool->freeIds, skipped);
      } else {
        assert(0 && "Free IDs array is full - cannot store skipped IDs");
      }
    }
    pool->nextId = newID + 1;
  }
  for (size_t i = 0; i < pool->freeIds.count; i++) {
    if (pool->freeIds.items[i] == newID) {
      pool->freeIds.items[i] = pool->freeIds.items[pool->freeIds.count - 1];
      pool->freeIds.count--;
      break;
    }
  }
  if (pool->entities_sparse.items[newID] != 0) {
    assert(0 && "add handling for if already occupyied -> need to remove newID "
                "from dense array before inserting or just insert on top of "
                "old one? and increment generation?");
  }

  // TODO: what if push fails?
  arr_push(&pool->entities_dense, entity);
  arr_insert(&pool->entities_sparse, newID, pool->entities_dense.count);
}
static void entityPool_push(EntityPool *pool, Entity entity) {
  en_id newID = _entityPool_GetNextId(pool);

  arr_push(&pool->entities_dense, entity);

  // if not 0 its not empty.
  // get generation value and increment it and add it to the entity being added.
  // en_gen generationId = 0;
  // en_identifier oldIdentifier = entityPool_get_from_dense(pool, newID);
  if (pool->entities_sparse.items[newID] != 0) {
    // increment the generationID;
    assert(0 && "Add handling for this");
  } else {
    arr_insert(&pool->entities_sparse, newID, pool->entities_dense.count);
  }
}

static bool entityPool_sparse_remove_idx(EntityPool *pool, en_id idx) {
  if (idx >= pool->entities_sparse.capacity) {
    assert(0 && "add logic to fix this?");
    return false; // Invalid index
  }

  if (pool->entities_sparse.items[idx] == 0) {
    assert(0 && "add logic to fix this?");
    return false; // Already empty
  }

  if (pool->freeIds.count >= pool->freeIds.capacity) {
    assert(0 && "add logic to fix this?");
    return false; // Free list is full
  }

  // TODO: this doesnt consider generation id
  pool->entities_sparse.items[idx] = 0;

  arr_push(&pool->freeIds, idx);
  return true;
}

static void entityPool_remove(EntityPool *pool, en_id idx) {
  size_t dense_idx_to_remove = pool->entities_sparse.items[idx];

  if (dense_idx_to_remove != pool->entities_dense.count - 1) {
    Entity last_entity =
        pool->entities_dense.items[pool->entities_dense.count - 1];
    en_id moved_entity_id = en_get_id(last_entity.identifier);

    pool->entities_sparse.items[moved_entity_id] = dense_idx_to_remove;
  }

  arr_remove_swap(&pool->entities_dense, dense_idx_to_remove);
  pool->entities_sparse.items[idx] = 0;
  arr_push(&pool->freeIds, idx);
}

int main(int argc, char *argv[]) {

  memory_arena arena = {0};
  size_t arenaSize = 1024 * 100;
  uint8_t *base = (uint8_t *)malloc(arenaSize);
  arena_init(&arena, base, arenaSize);

  EntityPool *ePool = entityPool_InitInArena(&arena, 1000);

  for (size_t i = 0; i < ePool->entities_dense.capacity; i++) {
    arr_insert(&ePool->entities_dense, i, (Entity){.identifier = i + 1});
  }
  for (size_t i = 0; i < ePool->entities_sparse.capacity; i++) {
    arr_insert(&ePool->entities_sparse, i, i + 1);
  }
  for (size_t i = 0; i < ePool->freeIds.capacity; i++) {
    arr_insert(&ePool->freeIds, i, i + 2);
  }
  ePool->nextId = 10;

  entityPool_clear(ePool);

  for (size_t i = 0; i < 50; i++) {
    entityPool_push(ePool,
                    (Entity){.identifier = _entity_id_create(0, 0, 0, 0)});
  }

  entityPool_remove(ePool, 25);
  // entityPool_insert(ePool,
  //                   (Entity){.identifier = _entity_id_create(5, 0, 0, 0)});
  // entityPool_push(ePool, (Entity){.identifier = _entity_id_create(2, 0, 0,
  // 0)}); entityPool_push(ePool, (Entity){.identifier = _entity_id_create(2, 0,
  // 0, 0)});
  //   entityPool_push(ePool, (Entity){.identifier = _entity_id_create(2, 0, 0,
  //   0)});
  size_t newid = _entityPool_GetNextId(ePool);
  en_identifier test = _entity_id_create(0xFFFF, 1, 2, 1);
  char buf[128];
  NUM_TO_BINARY_STR(buf, 128, test);
  printf("id  :      %s\n", buf);
  printf("hello world\n");
  // _increment_number_at_offset(test, 0, en_mask_id);

  size_t val2 = _get_number_at_offset(test, 0, en_mask_id);
  printf("value at ofseet %lli\n", val2);

  en_set_identifier_id(&test, 1);
  NUM_TO_BINARY_STR(buf, 128, test);
  printf("id  :      %s\n", buf);

  //
  // en_identifier val = test;
  // for (size_t i = 0; i < 255; i++) {
  //   val = _increment_number_at_offset(val, en_mask_flags_offset,
  //   en_mask_flags); NUM_TO_BINARY_STR(buf, 128, val); printf("id  : %s\n",
  //   buf);
  // }
  //
  // for (size_t i = 0; i < 0xFFFF; i++) {
  //   val = _increment_number_at_offset(val, en_mask_generation_offset,
  //                                     en_mask_generation);
  //   NUM_TO_BINARY_STR(buf, 128, val);
  //   printf("id  :      %s\n", buf);
  // }
  //
  // _increment_number_at_offset(&test, en_mask_flags_offset, en_mask_flags);
  NUM_TO_BINARY_STR(buf, 128, test);
  printf("\nid  :      %s\n", buf);

  return 0;
}
