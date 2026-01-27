#pragma once
#include "application_types.h"
#include "entityPool_types.h"
void spatialGrid_init(SpatialGrid *sGrid, arr_Entity *e);
void spatialGrid_update_dimensions(SpatialGrid *sGrid, Vector3 minBounds,
                                   Vector3 maxBounds, int spacing);

SpatialGrid *spatialGrid_create_with_dimensions(MemoryAllocator allocator,
                                                Vector3 minBounds,
                                                Vector3 maxBounds, int spacing,
                                                size_t maxEntities);
size_t spatialGrid_get_cell_index(const SpatialGrid *sGrid, Vector3 position);
size_t spatialGrid_get_entry_cell_index(const SpatialGrid *sGrid,
                                        const SpatialEntry *entry);
void spatial_populate(SpatialGrid *sGrid, EntityPool *entityPool,
                      MemoryAllocator *allocator);
