#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "entityPool_types.h"
typedef void *(*EntityPoolAllocFunc)(void *context, size_t size);
typedef struct {
  EntityPoolAllocFunc alloc; // Allocates 'size' bytes, returns NULL on failure
  void *context;             // Allocator-specific data (e.g., arena pointer)
} EntityPoolAllocator;
// ==================== ENTITY POOL ==================
EntityPool *entityPool_InitInArena(EntityPoolAllocator arena, size_t capacity);
void entityPool_clear(EntityPool *ePool);
en_identifier entity_id_create(uint32_t _idx, uint16_t _gen, uint16_t _spare,
                               uint8_t _flags);
void entityPool_remove(EntityPool *pool, en_id idx);
void entityPool_insert(EntityPool *pool, Entity entity);
void entityPool_push(EntityPool *pool, Entity entity);
Entity *entityPool_allocate_batch(EntityPool *pool, size_t count);
Entity *entityPool_get(EntityPool *pool, en_id id);
#ifdef ENTITY_POOL_DEVELOPMENT
en_id _entityPool_GetNextId(EntityPool *pool);
en_id _entityPool_PeekNextId(EntityPool *pool);
en_identifier _increment_number_at_offset(en_identifier idx, size_t offset,
                                          size_t mask);
en_identifier _get_number_at_offset(en_identifier idx, size_t offset,
                                    size_t mask);
en_id en_get_id(en_identifier idx);
en_gen en_get_generationid(en_identifier identifier);
void _set_number_at_offset(size_t value, size_t *num, size_t offset,
                           size_t mask);
void en_set_identifier_id(en_identifier *identifier, en_id value);
Entity *entityPool_from_dense_get_entity(EntityPool *pool, en_id idx);
en_identifier entityPool_from_dense_get_identifier(EntityPool *pool, en_id idx);

bool entityPool_sparse_remove_idx(EntityPool *pool, en_id idx);
#endif // ENTITY_POOL_DEVELOPMENT

// TODO: go trough this:

// ;compile
// World
// ```c
// #include <stdio.h>
// #include <string.h>

// static struct {
//     char msg[64];
// } hello (char const *name)
// {
//     typeof (hello(name)) ret = { "Hello" };
//     if (!name)
//         return ret;

//     size_t len = strnlen(name, sizeof ret.msg
//                          - 1 - sizeof "Hello");
//     if (!len)
//         return ret;

//     ret.msg[sizeof "Hello" - 1] = ' ';
//     memcpy(&ret.msg[sizeof "Hello"], name, len);

//     return ret;
// }

// int main (int, char **argv)
// {
//     puts(hello(argv[1]).msg);
// }
// ```
