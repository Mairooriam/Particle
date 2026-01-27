#pragma once
#define RAYMATH_IMPLEMENTATION
#include "internal/math/raymath.h"
#include <stdint.h>

typedef struct {
  Vector3 pos;
  Vector3 color;
} Vertex;

static inline void matrix_identity_init(Matrix *m) {
  // clang-format off
  *m = (Matrix){1.0f, 0.0f, 0.0f, 0.0f, 
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f, 
                0.0f, 0.0f, 0.0f, 1.0f};
  // clang-format on
};
static inline float deg_to_rad(float deg) { return deg * PI / 180.0f; }

typedef struct {
  Matrix *items;
  size_t capacity;
  size_t count;
} arr_Matrix;

typedef struct {
  uint32_t *items;
  size_t count;
  size_t capacity;
} arr_uint32_t;

typedef struct {
  size_t *items;
  size_t count;
  size_t capacity;
} arr_size_t;

typedef struct {
  Vector3 *items;
  size_t count;
  size_t capacity;
} arr_vec3f;

typedef struct {
  const char **items;
  size_t count;
  size_t capacity;
} arr_cstr;

typedef struct {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
} Color;
