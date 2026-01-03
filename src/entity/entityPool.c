#include "entityPool.h"
#include "entityPool_types.h"
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

#ifdef ENTITY_POOL_DEVELOPMENT
#define INTERNAL // Make functions visible for testing
#else
#define INTERNAL static // Hide functions in release
#endif
// ==================== INTERNAL IMPLEMENTATIONS ==================
INTERNAL en_id _entityPool_GetNextId(EntityPool *pool) {
  if (pool->freeIds.count > 0) {
    return pool->freeIds.items[--pool->freeIds.count];
  } else {
    return pool->nextId++; // return new id
  }
}

INTERNAL en_id _entityPool_PeekNextId(EntityPool *pool) {
  if (pool->freeIds.count > 0) {
    return pool->freeIds.items[pool->freeIds.count - 1];
  } else {
    return pool->nextId;
  }
}

// TODO: should i add overflow protection? 0xFF++ -> 0x00
// TODO: have it return it or edit the value?
// TODO: helpers that use this function? pass num++ as func? ->
// modify_number_at_offset(num, offset, mask, func)
INTERNAL en_identifier _increment_number_at_offset(en_identifier idx,
                                                   size_t offset, size_t mask) {
  size_t num = 0;
  num |= ((idx) >> offset) & mask;
  num++;
  idx = idx & ~(mask << offset);
  idx |= ((num) << offset);
  return idx;
}

INTERNAL en_identifier _get_number_at_offset(en_identifier idx, size_t offset,
                                             size_t mask) {
  return (idx >> offset) & mask;
}

INTERNAL en_id en_get_id(en_identifier idx) {
  return _get_number_at_offset(idx, 0, en_mask_id);
}

INTERNAL en_gen en_get_generationid(en_identifier identifier) {
  return _get_number_at_offset(identifier, en_mask_generation_offset,
                               en_mask_generation);
}
INTERNAL void _set_number_at_offset(size_t value, size_t *num, size_t offset,
                                    size_t mask) {
  *num = *num & ~(mask << offset);
  *num |= ((value) << offset);
}
INTERNAL void en_set_identifier_id(en_identifier *identifier, en_id value) {
  _set_number_at_offset(value, identifier, 0, en_mask_id);
}

INTERNAL Entity *entityPool_from_dense_get_entity(EntityPool *pool, en_id idx) {
  return &pool->entities_dense.items[pool->entities_sparse.items[idx]];
}
INTERNAL en_identifier entityPool_from_dense_get_identifier(EntityPool *pool,
                                                            en_id idx) {
  return pool->entities_dense.items[pool->entities_sparse.items[idx]]
      .identifier;
}

INTERNAL bool entityPool_sparse_remove_idx(EntityPool *pool, en_id idx) {
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

// ==================== PUBLIC API IMPLEMENTATIONS ==================
EntityPool *entityPool_InitInArena(memory_arena *arena, size_t capacity) {
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
void entityPool_clear(EntityPool *ePool) {
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

void entityPool_insert(EntityPool *pool, Entity entity) {
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

void entityPool_push(EntityPool *pool, Entity entity) {
  en_id newID = _entityPool_GetNextId(pool);
  en_set_identifier_id(&entity.identifier, newID);
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

void entityPool_remove(EntityPool *pool, en_id idx) {
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

// TODO: should parameters be bigger? if i change layout?
en_identifier entity_id_create(uint32_t _idx, uint16_t _gen, uint16_t _spare,
                               uint8_t _flags) {
  en_identifier idx = 0;
  idx = en_mask_id & _idx;
  idx |= (en_mask_generation & _gen) << en_mask_generation_offset;
  idx |= (en_mask_spare & _spare) << en_mask_spare_offset;
  idx |= (en_mask_flags & _flags) << en_mask_flags_offset;
  return idx;
}
void arena_init(memory_arena *arena, unsigned char *base,
                memory_index arena_size) {
  arena->base = base;
  arena->size = arena_size;
  arena->used = 0;
}

void *_PushStruct(memory_arena *arena, size_t size) {
  assert(arena->used + size <= arena->size);
  void *result = arena->base + arena->used;
  arena->used += size;
  memset(result, 0, size);
  return result;
}

Entity* entityPool_allocate_batch(EntityPool *pool, size_t count) {
    if (pool->entities_dense.count + count > pool->entities_dense.capacity) {
        return NULL; // Not enough space
    }
    
    Entity *first = &pool->entities_dense.items[pool->entities_dense.count];
    
    memset(first, 0, sizeof(Entity) * count);
    
    for (size_t i = 0; i < count; i++) {
        first[i].identifier = entity_id_create(_entityPool_GetNextId(pool), 0, 0, 0);
        en_id id = en_get_id(first[i].identifier);
        pool->entities_sparse.items[id] = pool->entities_dense.count + i;
    }
    
    pool->entities_dense.count += count;
    return first;
}