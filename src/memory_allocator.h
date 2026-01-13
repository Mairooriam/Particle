#pragma once
#include <string.h>  // For memset
#include <assert.h>
#define ALIGNMENT 16  // Common alignment for safety (adjust to 8 or 32 if needed for your platform/types)

// Internal: Align a size to the next multiple of ALIGNMENT
static inline size_t align_size(size_t size) {
    return (size + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
}
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
    assert(arena != NULL); 
    assert(base != NULL); 
    assert(arena_size > 0); 

  arena->base = base;
  arena->size = arena_size;
  arena->used = 0;
}
static inline void arena_reset(memory_arena *arena) {
   assert(arena != NULL);  
  arena->used = 0;
}
#define arena_PushStruct(Arena, type) (type *)_PushStruct(Arena, sizeof(type))
#define arena_PushStructs(Arena, type, count) \
  (type *)_PushStruct(Arena, sizeof(type) * count)

// Internal 
static void *_PushStruct(memory_arena *arena, size_t size) {
    if (!arena || size == 0 || size > SIZE_MAX / 2) {  
        assert(0 && "Invalid arena or size in _PushStruct");
        return NULL;
    }
    
    size_t aligned_size = align_size(size);
    
    if (arena->used > arena->size || aligned_size > arena->size - arena->used) {
        assert(0 && "Arena out of memory or overflow in _PushStruct");
        return NULL;
    }
    
    void *result = arena->base + arena->used;
    arena->used += aligned_size;
    memset(result, 0, size); 
    return result;
}
static void* arena_alloc(void* context, size_t size) {
    memory_arena* arena = (memory_arena*)context;
    if (!arena) {
        assert(0 && "Null arena context in arena_alloc");
        return NULL;
    }
    
    printf("arena_alloc: requested size=%zu, used=%zu, total=%zu\n",
           size, arena->used, arena->size);
    
    if (arena->used + align_size(size) > arena->size) {
        printf("arena_alloc: Out of memory! Requested=%zu (aligned), Available=%zu\n",
               align_size(size), arena->size - arena->used);
        assert(0 && "Arena out of memory in arena_alloc");
        return NULL;
    }
    
    return _PushStruct(arena, size);
}


static void arena_free(void* context, void* ptr) {
   assert(context != NULL);
    (void)context;
    (void)ptr;
}

static MemoryAllocator create_arena_allocator(memory_arena* arena) {
    if (!arena) {
    // Log error instead of assert for release builds
    fprintf(stderr, "Error: NULL arena passed to create_arena_allocator\n");
    MemoryAllocator invalid = {NULL, NULL, NULL};
    assert(0 && "lol no arena to be found");
    return invalid;
  }
    MemoryAllocator allocator;
    allocator.alloc = arena_alloc;
    allocator.free = arena_free;
    allocator.context = arena;
    return allocator;
}

