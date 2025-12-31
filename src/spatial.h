#pragma once

typedef struct {
  int bX, bY; // Bounds x and y
  int spacing;
  size_t capacity;
  size_t *entitiesDense;
  size_t entitiesDenseCount;
  size_t *entitiesSparse;
  size_t entitiesSpareCount;
  size_t numY; // number of cells in Y
  size_t numX; // number of cells in X
} SpatialGrid;

SpatialGrid *SpatialGrid_create(int bounds_x, int bounds_y, int spacing,
                                size_t entityCount);

typedef struct Entities Entities;
void update_spatial(SpatialGrid *sGrid, Entities *e);
