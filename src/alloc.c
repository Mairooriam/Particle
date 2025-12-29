#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#define Assert(Expression)                                                     \
  if (!(Expression)) {                                                         \
    *(int *)0 = 0;                                                             \
  }
#define KiloBytes(value) ((value) * 1024)
#define MegaBytes(value) ((KiloBytes(value)) * 1024)
#define GigaBytes(value) ((MegaBytes(value)) * 1024)
#define TeraBytes(value) ((GigaBytes(value)) * 1024)

typedef struct {
  bool isInitialized;
  void *permamentMemory;
  size_t permanentMemorySize;
  void *transientMemory;
  size_t transientMemorySize;
} GameMemory;
// RENDER COMPONENT
typedef struct {
  float r, g, b, a;
  float renderRadius; // Visual size (can differ from collision radius)
} c_Render;

// TRANSFORM COMPONENT
typedef struct {
  float pX, pY, pZ;
  float vX, vY, vZ;  // Velocity
  float aX, aY, aZ;  // acceleration
  float restitution; // 1 -> will bounce apart - 0 -> both will keep moving to
                     // same direction
} c_Transform;

typedef struct {
  float springConstat;
  float restLenght;
  size_t parent;
} c_Spring;

// COLLISION COMPONENT
typedef struct {
  float radius;
  float mass;
  float inverseMass;
  size_t collisionCount;
  size_t searchCount;
  float faX, faY, faZ; // force accum
} c_Collision;
typedef struct Entity Entity;
typedef struct Entity {
  size_t id;
  uint64_t flags;
  c_Transform c_transform;
  c_Render c_render;
  c_Collision c_collision;
  Entity *spawnEntity;
  float spawnCount;
  float spawnRate;
  float clock;
  bool followMouse;
  c_Spring c_spring;
} Entity;
typedef struct Entities {
  Entity *items;
  size_t count;
  size_t capacity;
} Entities;
static void ConcatStrings(size_t sourceACount, char *sourceAstr,
                          size_t sourceBCount, char *sourceBstr,
                          size_t destCount, char *destStr) {
  (void)destCount;
  // TOOO: overflowing stuff
  for (size_t i = 0; i < sourceACount; i++) {
    *destStr++ = *sourceAstr++;
  }
  for (size_t i = 0; i < sourceBCount; i++) {
    *destStr++ = *sourceBstr++;
  }
  *destStr++ = 0;
}

// typedef struct {
//  // is it used
//   // pointer to next free
//
//   uintptr_t *nextFree;
// } Header;
//
// typedef struct {
//
// } Block;
//
// typedef struct {
//  // size of one block
// } Pool;
typedef struct {
  char *base;
  size_t size;
  size_t used;
} memory_arena;

memory_arena arena_init(void *buffer, size_t arena_size) {
  memory_arena arena = {0};
  arena.base = (char *)buffer;
  arena.size = arena_size;
  arena.used = 0;
  return arena;
}

void *arena_alloc(memory_arena *arena, void *data, size_t size) {
  (void)data;
  // (char*)base + used amount of bytes;
  *(arena->base + arena->used) = *(char *)data;
  arena->used += size;
  void *ptr = arena->base + arena->used;
  // size_t mask = 7; // aligned to 8-1 for mask
  // size_t aligned_used = (arena->used + mask) & ~mask;
  // Assert(aligned_used + size <= arena->size);
  // if (aligned_used + size > arena->size) {
  //   return NULL;
  // }
  // void *ptr = arena->base + aligned_used;
  // arena->used = aligned_used + size;

  return ptr;
}

int arena_free(memory_arena *arena) {
  arena->used = 0;
  memset(arena->base, 0, arena->size);
  return 0;
}
static inline void Entities_init_with_buffer(Entities *da, size_t cap,
                                             Entity *buffer) {
  da->count = 0;
  da->capacity = cap;
  da->items = buffer;
}
static void entity_add(Entities *entities, Entity entity) {
  if (!entities) {
    fprintf(stderr, "Entities pointer is NULL\n");
    return;
  }
  if (entities->count >= entities->capacity) {
    fprintf(stderr, "Entities buffer is full, cannot add more entities\n");
    return;
  }
  entity.id = entities->count;
  entities->items[entities->count++] = entity;
}
static void entity_add_to_arena(Entity entity, memory_arena *arena) {
  arena_alloc(arena, &entity, sizeof(Entity));
}
static void entity_add_to_buf(Entity entity, void *buf) {}
int main(int argc, char const *argv[]) {
  LPVOID baseAddress = (LPVOID)TeraBytes((uint64_t)2);

  GameMemory gameMemory = {0};
  gameMemory.permanentMemorySize = MegaBytes(64);
  gameMemory.transientMemorySize = GigaBytes((uint64_t)1);

  uint64_t totalSize =
      gameMemory.permanentMemorySize + gameMemory.transientMemorySize;
  gameMemory.permamentMemory = VirtualAlloc(
      baseAddress, totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (!gameMemory.permamentMemory) {
    printf("erorr");
  }
  gameMemory.transientMemory =
      ((uint8_t *)gameMemory.permamentMemory + gameMemory.permanentMemorySize);

  printf("totalsize: %zu, permanentMemorySize: %zu, transientMemorySize: %zu\n",
         totalSize, gameMemory.permanentMemorySize,
         gameMemory.transientMemorySize);

  size_t entitiesCount = 1000;
  size_t entititesSize = sizeof(Entities) + (sizeof(Entity) * entitiesCount);
  printf("Sizeof  single entity %zu\n", sizeof(Entity));
  printf("Entities count %zu\n", entitiesCount);
  printf("sizeof Entities array %zu\n", sizeof(Entities));
  printf("sizeof only Entities %zu\n", sizeof(Entity) * entitiesCount);
  printf("EntitiesSize %zu\n", entititesSize);

  memory_arena arena = arena_init(gameMemory.permamentMemory, entititesSize);
  // arena_alloc(arena, size_t size)
  {
    Entity e = {0};
    for (size_t i = 0; i < entitiesCount; i++) {
      e.id = i + 1;
      arena_alloc(&arena, &e, sizeof(Entity));
    }
  }

  // Entities_init_with_buffer(&entities, entitiesCount, entitiesBuffer);

  // entity_add(&entities, e);
  // e.id = 200;
  // entity_add(&entities, e);
  Entity *firstEntity = (Entity *)arena.base;
  printf("Arena first entity id: %zu\n", firstEntity->id);
  Entity *secondEntity = firstEntity + 1;
  printf("Arena second entity id: %zu\n", secondEntity->id);
  printf("Arena second entity id: %zu\n", firstEntity->id);
  printf("Arena size: %zu, Arena used: %zu\n", arena.size, arena.used);
  for (Entity *e = (Entity *)arena.base; e; e++) {
    printf("Printing arena contents. ENTITY id: %zu\n", e->id);
  }
  return 0;
}
