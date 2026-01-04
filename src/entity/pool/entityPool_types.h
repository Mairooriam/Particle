#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include "entity_types.h"
typedef uint64_t en_identifier;
typedef uint32_t en_id;
typedef uint16_t en_gen;

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

// static void strrev_impl(char *head) {
//   if (!head)
//     return;
//   char *tail = head;
//   while (*tail)
//     ++tail; // find the 0 terminator, like head+strlen
//   --tail;   // tail points to the last real char
//             // head still points to the first
//   for (; head < tail; ++head, --tail) {
//     // walk pointers inwards until they meet or cross in the middle
//     char h = *head, t = *tail;
//     *head = t; // swapping as we go
//     *tail = h;
//   }
// }

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

#ifdef SLOW_CODE_ALLOWED
#define arr_get_safe(arr, idx)                                                 \
  (assert((idx) < (arr)->count && "Index out of bounds"), (arr)->items[(idx)])
#else
#define arr_get_safe(arr, idx) ((arr)->items[(idx)])
#endif


typedef struct {
  en_id *items;
  size_t count;
  size_t capacity;
} arr_entityID;

typedef struct {
  arr_entityID entities_sparse; // stores indicies for entities located in dense
  arr_Entity entities_dense;    // contigious entity arr
  size_t capacity; // Total capacity of pool. other arrays share the same size
  arr_entityID freeIds;
  size_t nextId; // Next unique ID
} EntityPool;
