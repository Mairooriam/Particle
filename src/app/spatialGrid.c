#include "spatialGrid.h"

void spatialGrid_update_dimensions(SpatialGrid *sGrid, Vector3 minBounds,
                                   Vector3 maxBounds, int spacing);
void spatialGrid_init(SpatialGrid *sGrid, arr_Entity *e);
void spatialGrid_update_dimensions(SpatialGrid *sGrid, Vector3 minBounds,
                                   Vector3 maxBounds, int spacing);
void spatialGrid_init(SpatialGrid *sGrid, arr_Entity *e);
SpatialGrid *spatialGrid_create_with_dimensions(MemoryAllocator allocator,
                                                Vector3 minBounds,
                                                Vector3 maxBounds, int spacing,
                                                size_t maxEntities) {
  SpatialGrid *sGrid =
      (SpatialGrid *)allocator.alloc(allocator.context, sizeof(SpatialGrid));

  // GRID DETAILS
  sGrid->spacing = spacing;
  sGrid->bX = (int)(maxBounds.x - minBounds.x);
  sGrid->bY = (int)(maxBounds.y - minBounds.y);
  sGrid->numX = sGrid->bX / spacing;
  sGrid->numY = sGrid->bY / spacing;
  sGrid->capacity = maxEntities;

  // DENSE
  sGrid->spatialDense.items = (SpatialEntry *)allocator.alloc(
      allocator.context, sizeof(SpatialEntry) * maxEntities);
  sGrid->spatialDense.count = 0;
  sGrid->spatialDense.capacity = maxEntities;

  // SPARSE
  size_t gridCells = sGrid->numX * sGrid->numY;
  sGrid->spatialSparse.items =
      (size_t *)allocator.alloc(allocator.context, sizeof(size_t) * gridCells);
  sGrid->spatialSparse.capacity = gridCells;
  sGrid->spatialSparse.count = 0;

  // CELL COUNTS
  sGrid->cellCounts.items =
      (size_t *)allocator.alloc(allocator.context, sizeof(size_t) * gridCells);
  sGrid->cellCounts.capacity = gridCells;
  sGrid->cellCounts.count = 0;

  sGrid->isInitalized = true;
  return sGrid;
}

void spatialGrid_update_dimensions(SpatialGrid *sGrid, Vector3 minBounds,
                                   Vector3 maxBounds, int spacing) {
  assert(!sGrid->isInitalized && "Initialize first before updating");

  sGrid->spacing = spacing;
  sGrid->bX = (int)(maxBounds.x - minBounds.x);
  sGrid->bY = (int)(maxBounds.y - minBounds.y);
  sGrid->numX = sGrid->bX / spacing;
  sGrid->numY = sGrid->bY / spacing;
}
static inline size_t spatialGrid_get_cell_index(const SpatialGrid *sGrid,
                                                Vector3 position) {
  int cellX = (int)(position.x / sGrid->spacing);
  int cellY = (int)(position.y / sGrid->spacing);

  if (cellX < 0 || cellX >= (int)sGrid->numX || cellY < 0 ||
      cellY >= (int)sGrid->numY) {
    return (size_t)-1;
  }

  return cellY * sGrid->numX + cellX;
}

size_t spatialGrid_get_entry_cell_index(const SpatialGrid *sGrid,
                                        const SpatialEntry *entry) {
  return spatialGrid_get_cell_index(sGrid, entry->position);
}
static void _spatialGrid_populate(SpatialGrid *sGrid, arr_vec3f *positions,
                                  arr_size_t *ids) {
  size_t numY = sGrid->numY;

  sGrid->spatialDense.count = 0;
  memset(sGrid->spatialSparse.items, 0,
         sizeof(size_t) * sGrid->spatialSparse.capacity);
  if (sGrid->cellCounts.items) {
    memset(sGrid->cellCounts.items, 0,
           sizeof(size_t) * sGrid->cellCounts.capacity);
  }

  // PART ONE: fill sparse with the entitie counts AND push into dense
  size_t count = 0;
  for (size_t i = 0; i < positions->count; i++) {
    SpatialEntry entry =
        (SpatialEntry){.entityId = ids->items[i],
                       .position = positions->items[i],
                       .entityPoolIndex = 0}; // TODO: for now 0 not used yet
    size_t sparseID = spatialGrid_get_entry_cell_index(sGrid, &entry);
    if (sparseID == (size_t)-1)
      continue;

    sGrid->spatialDense.items[count] = entry;
    sGrid->spatialSparse.items[sparseID] = count + 1;
    count++;
  }
  sGrid->spatialDense.count = count;

  // PART TWO: partial sums
  size_t currentSum = 0;
  for (size_t i = 0; i < sGrid->spatialSparse.capacity; i++) {
    size_t num = sGrid->spatialSparse.items[i];
    if ((currentSum != num) && (num != 0)) {
      currentSum = num;
    }
    sGrid->spatialSparse.items[i] = currentSum;
  }

  // PART THREE: iterate trough dense entries and decrement sparse
  for (size_t i = 0; i < sGrid->spatialDense.count; i++) {
    SpatialEntry *entry = &sGrid->spatialDense.items[i];
    size_t sparseID = spatialGrid_get_entry_cell_index(sGrid, entry);

    sGrid->spatialSparse.items[sparseID]--;
  }
}

void spatial_populate(SpatialGrid *sGrid, EntityPool *entityPool,
                      MemoryAllocator *allocator) {
  arr_vec3f positions = {0};
  arr_size_t ids = {0};

  positions.items = (Vector3 *)allocator->alloc(
      allocator->context, sizeof(Vector3) * entityPool->entities_dense.count);
  positions.capacity = entityPool->entities_dense.count;
  positions.count = 0;

  ids.items = (size_t *)allocator->alloc(
      allocator->context, sizeof(size_t) * entityPool->entities_dense.count);
  ids.capacity = entityPool->entities_dense.count;
  ids.count = 0;

  for (size_t i = 0; i < entityPool->entities_dense.count; i++) {
    Entity *e = &entityPool->entities_dense.items[i];

    if (!(e->flags & ENTITY_FLAG_HAS_TRANSFORM))
      continue;

    positions.items[positions.count] = e->c_transform.pos;
    ids.items[ids.count] = e->identifier;
    positions.count++;
    ids.count++;
  }

  _spatialGrid_populate(sGrid, &positions, &ids);
}
