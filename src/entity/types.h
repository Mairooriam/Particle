#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef uint64_t en_identifier;
typedef uint32_t en_id;

typedef struct {
  float x, y;
} Vector2;

typedef struct {
  bool needsGridUpdate;
} c_Transform;

typedef struct Entity {
  en_identifier identifier;
  Vector2 position;
  c_Transform transform;
} Entity;
