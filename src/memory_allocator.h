#pragma once
#include <string.h>  // For memset
#include <assert.h>

typedef unsigned char      mem_base;
typedef struct memory_arena {
    mem_base* base;
    size_t size;
    size_t used;
} memory_arena;

// Generic memory allocator interface
typedef struct {
    void* (*alloc)(void* context, size_t size);  // Allocates 'size' bytes, returns NULL on failure
    void (*free)(void* context, void* ptr);      // Frees the pointer (optional, can be NULL for arenas)
    void* context;                               // Allocator-specific data (e.g., arena pointer)
} MemoryAllocator;

#define ALLOC_DA(allocator, da, type, cap) do { \
    da.items = (type*)allocator.alloc(allocator.context, sizeof(type) * (cap)); \
    if (!da.items) return NULL; \
    da.capacity = (cap); \
    da.count = 0; \
} while(0)

#define memory_index size_t  

void arena_init(memory_arena *arena, mem_base *base, memory_index arena_size) {
  arena->base = base;
  arena->size = arena_size;
  arena->used = 0;
}
static inline void arena_reset(memory_arena *arena) {
  arena->used = 0;
}
#define arena_PushStruct(Arena, type) (type *)_PushStruct(Arena, sizeof(type))
#define arena_PushStructs(Arena, type, count) \
  (type *)_PushStruct(Arena, sizeof(type) * count)

// Internal 
static void *_PushStruct(memory_arena *arena, size_t size) {
  assert(arena->used + size <= arena->size);  
  void *result = arena->base + arena->used;
  arena->used += size;
  memset(result, 0, size);
  return result;
}

static void* arena_alloc(void* context, size_t size) {
    memory_arena* arena = (memory_arena*)context;
    return _PushStruct(arena, size);
}

static void arena_free(void* context, void* ptr) {
    (void)context;
    (void)ptr;
}

static MemoryAllocator create_arena_allocator(memory_arena* arena) {
    MemoryAllocator allocator;
    allocator.alloc = arena_alloc;
    allocator.free = arena_free;
    allocator.context = arena;
    return allocator;
}

