#pragma once

#include "stdint.h"
#include "utils.h"

typedef struct {
  size_t *items;
  size_t count;
  size_t capacity;
} arr_size_t;
DA_CREATE(arr_size_t)
DA_FREE(arr_size_t)
DA_INIT(arr_size_t)

// |------------------------------| ->> array of pointers size_t**
// [[0][1][2][3][4][5][6][7][8][9]]
//   ^
//   pointer to size_t*
typedef struct {
  arr_size_t *items;
  size_t count;
  size_t capacity;
} arr_size_t_ptr;
DA_CREATE(arr_size_t_ptr)
DA_FREE(arr_size_t_ptr)
DA_INIT(arr_size_t_ptr)
