#pragma once
#include "shared.h"
#include <stdint.h>

typedef struct {
  size_t *items;
  size_t count;
  size_t capacity;
} arr_size_t;

#include "entity_types.h"

// ==================== SPATIAL ====================
typedef struct {
  int bX, bY; // Bounds x and y
  int spacing;
  size_t capacity;
  arr_size_t spatialDense;
  arr_size_t spatialSparse;
  size_t numY; // number of cells in Y
  size_t numX; // number of cells in X
  bool isInitalized;
} SpatialGrid;

void spatialGrid_update_dimensions(SpatialGrid *sGrid, Vector3 minBounds,
                                   Vector3 maxBounds, int spacing);
void update_spatial(SpatialGrid *sGrid, arr_Entity *e);
