#pragma once
#include "shared.h"
#include <stdint.h>

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

void spatialGrid_update_dimensions(SpatialGrid *sGrid, Vector3 minBounds,
                                   Vector3 maxBounds, int spacing);
void spatialGrid_init(SpatialGrid *sGrid, arr_Entity *e);
