#pragma once
#include "spatial.h"
#include "components.h"
#include "stdlib.h"
#include "types.h"
#include <assert.h>
#include <math.h>
#include <string.h>
SpatialGrid *SpatialGrid_create(int bounds_x, int bounds_y, int spacing,
                                size_t entityCount) {
  SpatialGrid *sGrid = malloc(sizeof(SpatialGrid));
  if (!sGrid) {
    assert(0 && "LMAO no memory");
  }

  sGrid->bX = bounds_x;
  sGrid->bY = bounds_y;
  sGrid->spacing = spacing;
  sGrid->antitiesDense = *arr_size_t_create(entityCount);
  sGrid->numY = bounds_y / spacing;
  sGrid->numX = bounds_x / spacing;
  sGrid->entities = *arr_size_t_create(sGrid->numX * sGrid->numY + 1);
  return sGrid;
}

void update_spatial(SpatialGrid *sGrid, Entities *e) {
  float spacing = sGrid->spacing;

  // Clear arrays
  memset(sGrid->entities.items, 0,
         sGrid->entities.capacity * sizeof(sGrid->entities.items[0]));
  sGrid->antitiesDense.count = 0;

  // First pass: count entities per cell
  for (size_t i = 0; i < e->entitiesCount; i++) {
    Component_transform *cT1 = &e->c_transform->items[i];

    float xi_f = cT1->pos.x / spacing;
    float yi_f = cT1->pos.y / spacing;
    if (xi_f < 0 || yi_f < 0)
      continue;

    size_t xi = (size_t)floorf(xi_f);
    size_t yi = (size_t)floorf(yi_f);

    if (xi >= sGrid->numX || yi >= sGrid->numY)
      continue;

    size_t idx = xi * sGrid->numY + yi;
    if (idx >= sGrid->entities.capacity)
      continue;

    // Count entities in this cell
    sGrid->entities.items[idx]++;
  }

  // Prefix sum to get start indices
  size_t sum = 0;
  for (size_t i = 0; i < sGrid->entities.capacity; i++) {
    size_t count = sGrid->entities.items[i];
    sGrid->entities.items[i] = sum; // Store start index
    sum += count;
  }

  // Resize dense array to fit all entities
  // sGrid->antitiesDense.count = sum;

  // Second pass: fill dense array
  for (size_t i = 0; i < e->entitiesCount; i++) {
    Component_transform *cT1 = &e->c_transform->items[i];

    float xi_f = cT1->pos.x / spacing;
    float yi_f = cT1->pos.y / spacing;
    if (xi_f < 0 || yi_f < 0)
      continue;

    size_t xi = (size_t)floorf(xi_f);
    size_t yi = (size_t)floorf(yi_f);

    if (xi >= sGrid->numX || yi >= sGrid->numY)
      continue;

    size_t idx = xi * sGrid->numY + yi;
    if (idx >= sGrid->entities.capacity)
      continue;

    // Add entity to dense array at the next available position for this cell
    size_t denseIdx = sGrid->entities.items[idx];
    sGrid->antitiesDense.items[denseIdx] = i;
    sGrid->entities.items[idx]++; // Increment for next entity in this cell
  }

  // Restore start indices (optional, needed for querying)
  sum = 0;
  for (size_t i = 0; i < sGrid->entities.capacity; i++) {
    size_t end = sGrid->entities.items[i];
    sGrid->entities.items[i] = sum;
    sum = end;
  }
}
