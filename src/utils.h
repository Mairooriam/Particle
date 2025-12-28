#pragma once
#include "stdlib.h"
#ifdef ENABLE_TIMING
typedef struct {
  const char *name;
  double total_ms;
  int count;
} TimingEntry;

extern TimingEntry g_timing_entries[32];
extern int g_timing_count;
extern double g_timing_accumulated_time;
extern double g_timing_print_interval; // Print every N seconds

#define TIME_IT(label, code)                                                   \
  do {                                                                         \
    double _start = GetTime();                                                 \
    code;                                                                      \
    double _elapsed = (GetTime() - _start) * 1000.0;                           \
    if (g_timing_count < 32) {                                                 \
      g_timing_entries[g_timing_count].name = label;                           \
      g_timing_entries[g_timing_count].total_ms = _elapsed;                    \
      g_timing_entries[g_timing_count].count = 1;                              \
      g_timing_count++;                                                        \
    }                                                                          \
  } while (0)

#define TIMING_FRAME_BEGIN()                                                   \
  do {                                                                         \
    g_timing_count = 0;                                                        \
  } while (0)

#define TIMING_FRAME_END(frametime)                                            \
  do {                                                                         \
    g_timing_accumulated_time += frametime;                                    \
    if (g_timing_accumulated_time >= g_timing_print_interval) {                \
      for (int i = 0; i < g_timing_count; i++) {                               \
        printf("%s: %.3f ms\n", g_timing_entries[i].name,                      \
               g_timing_entries[i].total_ms);                                  \
      }                                                                        \
      printf("--- (%.2fs elapsed)\n", g_timing_accumulated_time);              \
      g_timing_accumulated_time = 0.0;                                         \
    }                                                                          \
  } while (0)

#define TIMING_SET_INTERVAL(seconds)                                           \
  do {                                                                         \
    g_timing_print_interval = seconds;                                         \
  } while (0)
#else
#define TIME_IT(label, code)                                                   \
  do {                                                                         \
    code;                                                                      \
  } while (0)
#define TIMING_FRAME_BEGIN()
#define TIMING_FRAME_END(frametime)
#define TIMING_SET_INTERVAL(seconds)
#endif

#define DA_CREATE(type)                                                        \
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
  }

#define DA_FREE(type)                                                          \
  static inline void type##_free(type *da) {                                   \
    if (da) {                                                                  \
      free(da->items);                                                         \
      free(da);                                                                \
    }                                                                          \
  }

#define DA_INIT(type)                                                          \
  static inline void type##_init(type *da, size_t cap) {                       \
    da->count = 0;                                                             \
    da->capacity = cap;                                                        \
    da->items = malloc(sizeof(*da->items) * cap);                              \
  }

#define NOB_REALLOC realloc
#define NOB_ASSERT(Expression)                                                 \
  if (!(Expression)) {                                                         \
    *(int *)0 = 0;                                                             \
  }
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

#define nob_da_last(da)                                                        \
  (da)->items[(NOB_ASSERT((da)->count > 0), (da)->count - 1)]
#define nob_da_remove_unordered(da, i)                                         \
  do {                                                                         \
    size_t j = (i);                                                            \
    NOB_ASSERT(j < (da)->count);                                               \
    (da)->items[j] = (da)->items[--(da)->count];                               \
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

#define FLAG_SET(flags, flag)                                                  \
  ((flags) |=                                                                  \
   (flag)) // Example: FLAG_SET(entity.flags, FLAG_ACTIVE | FLAG_VISIBLE);
#define FLAG_CLEAR(flags, flag)                                                \
  ((flags) &= ~(flag)) // Example: FLAG_CLEAR(entity.flags, FLAG_COLLIDING);
#define FLAG_TOGGLE(flags, flag)                                               \
  ((flags) ^= (flag)) // Example: FLAG_TOGGLE(entity.flags, FLAG_VISIBLE);
#define FLAG_IS_SET(flags, flag)                                               \
  (((flags) & (flag)) !=                                                       \
   0) // Example: if (FLAG_IS_SET(entity.flags, FLAG_ACTIVE)) { ... }
#define FLAG_ALL_SET(flags, mask)                                              \
  (((flags) & (mask)) == (mask)) // Example: if (FLAG_ALL_SET(entity.flags,
                                 // FLAG_ACTIVE | FLAG_VISIBLE)) { ... }
#define FLAG_ANY_SET(flags, mask)                                              \
  (((flags) & (mask)) != 0) // Example: if (FLAG_ANY_SET(entity.flags,
                            // FLAG_COLLIDING | FLAG_DEAD)) { ... }
#define FLAG_CLEAR_ALL(flags) ((flags) = FLAG_NONE)
