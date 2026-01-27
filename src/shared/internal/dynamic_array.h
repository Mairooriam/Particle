#pragma once

// ==================== DA FUNC DECL ====================
#define DA_DEFINE(type)                                                        \
  static inline type *type##_create(size_t cap) {                              \
    type *da = malloc(sizeof(type));                                           \
    if (!da)                                                                   \
      return NULL;                                                             \
    da->count = 0;                                                             \
    da->capacity = cap;                                                        \
    da->items = malloc(sizeof(*da->items) * cap);                              \
    if (!da->items) {                                                          \
      free(da);                                                                \
      return NULL;                                                             \
    }                                                                          \
    return da;                                                                 \
  }                                                                            \
                                                                               \
  static inline void type##_free(type *da) {                                   \
    if (da) {                                                                  \
      free(da->items);                                                         \
      free(da);                                                                \
    }                                                                          \
  }                                                                            \
                                                                               \
  static inline void type##_init(type *da, size_t cap) {                       \
    da->count = 0;                                                             \
    da->capacity = cap;                                                        \
    da->items = malloc(sizeof(*da->items) * cap);                              \
    if (!da->items) {                                                          \
      da->capacity = 0;                                                        \
    }                                                                          \
  }

#define DA_DEFINE_ALLOCATOR(type, allocator)                                   \
  static inline type *type##_create_with_allocator(size_t cap, MemoryAllocator *allocator) { \
    type *da = (type *)allocator->alloc(allocator->context, sizeof(type));     \
    if (!da)                                                                   \
      return NULL;                                                             \
    da->count = 0;                                                             \
    da->capacity = cap;                                                        \
    da->items = (typeof(da->items))allocator->alloc(allocator->context,        \
                                                    sizeof(*da->items) * cap); \
    if (!da->items) {                                                          \
      allocator->free(allocator->context, da);                                 \
      return NULL;                                                             \
    }                                                                          \
    return da;                                                                 \
  }                                                                            \
                                                                               \
  static inline void type##_free_with_allocator(type *da, MemoryAllocator *allocator) { \
    if (da) {                                                                  \
      allocator->free(allocator->context, da->items);                          \
      allocator->free(allocator->context, da);                                 \
    }                                                                          \
  }

// ==================== DA OPERATIONS ====================
#define NOB_REALLOC realloc
#define NOB_ASSERT(c)                                                          \
  do {                                                                         \
    if (!(c)) {                                                                \
      __builtin_trap();                                                        \
    }                                                                          \
  } while (0)
#ifndef NOB_DA_INIT_CAP
#define NOB_DA_INIT_CAP 256
#endif

#ifdef __cplusplus
#define NOB_DECLTYPE_CAST(T) (decltype(T))
#else
#define NOB_DECLTYPE_CAST(T)
#endif // __cplusplus

#define nob_da_reserve(da, expected_capacity)                                  \
  do {                                                                         \
    if ((expected_capacity) > (da)->capacity) {                                \
      if ((da)->capacity == 0) {                                               \
        (da)->capacity = NOB_DA_INIT_CAP;                                      \
      }                                                                        \
      while ((expected_capacity) > (da)->capacity) {                           \
        (da)->capacity *= 2;                                                   \
      }                                                                        \
      (da)->items = NOB_DECLTYPE_CAST((da)->items)                             \
          NOB_REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items));     \
      NOB_ASSERT((da)->items != NULL);                                         \
    }                                                                          \
  } while (0)

// Append an item to a dynamic array
#define nob_da_append(da, item)                                                \
  do {                                                                         \
    nob_da_reserve((da), (da)->count + 1);                                     \
    (da)->items[(da)->count++] = (item);                                       \
  } while (0)

#define nob_da_free(da) NOB_FREE((da).items)

// Append several items to a dynamic array
#define nob_da_append_many(da, new_items, new_items_count)                     \
  do {                                                                         \
    nob_da_reserve((da), (da)->count + (new_items_count));                     \
    memcpy((da)->items + (da)->count, (new_items),                             \
           (new_items_count) * sizeof(*(da)->items));                          \
    (da)->count += (new_items_count);                                          \
  } while (0)

#define nob_da_resize(da, new_size)                                            \
  do {                                                                         \
    nob_da_reserve((da), new_size);                                            \
    (da)->count = (new_size);                                                  \
  } while (0)




// Foreach over Dynamic Arrays. Example:
// ```c
// typedef struct {
//     int *items;
//     size_t count;
//     size_t capacity;
// } Numbers;
//
// Numbers xs = {0};
//
// nob_da_append(&xs, 69);
// nob_da_append(&xs, 420);
// nob_da_append(&xs, 1337);
//
// nob_da_foreach(int, x, &xs) {
//     // `x` here is a pointer to the current element. You can get its index by
//     taking a difference
//     // between `x` and the start of the array which is `x.items`.
//     size_t index = x - xs.items;
//     nob_log(INFO, "%zu: %d", index, *x);
// }
// ```
#define nob_da_foreach(Type, it, da)                                           \
  for (Type *it = (da)->items; it < (da)->items + (da)->count; ++it)




