#pragma once
#include "shared.h"
#include <stdint.h>

#include "entity_types.h"
// ==================== SPATIAL ====================
typedef struct {
  size_t entityPoolIndex; // TODO: for future linkin to entityPool dense idx (
                          // actual entity data )
  Vector3 position;       // TODO: for future use
  size_t entityId;
} SpatialEntry;

// Dynamic array of spatial entries
typedef struct {
  SpatialEntry *items;
  size_t count;
  size_t capacity;
} arr_SpatialEntry;

// Spatial grid structure
typedef struct {
  int bX, bY;
  int spacing;
  size_t capacity;
  arr_SpatialEntry spatialDense;
  arr_size_t spatialSparse;
  arr_size_t cellCounts; // TODO: for future use
  size_t numY;
  size_t numX;
  bool isInitalized;
} SpatialGrid;

