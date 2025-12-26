#pragma once

#include "types.h"
typedef struct {
  int bX, bY; // Bounds x and y
  int spacing;
  arr_size_t antitiesDense;
  arr_size_t entities;
  size_t numY; // number of cells in Y
  size_t numX; // number of cells in X
} SpatialGrid;
typedef struct Entities Entities;
SpatialGrid *SpatialGrid_create(int bounds_x, int bounds_y, int spacing,
                                Entities *e);

void update_spatial(SpatialGrid *sGrid, Entities *e);
