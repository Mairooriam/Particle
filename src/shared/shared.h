#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "internal/dynamic_array.h"

#include "internal/mirMath.h"
#include "internal/memory_allocator.h"
#include "internal/renderQue.h"

#define Assert(expression)                                                     \
  if (!(expression)) {                                                         \
    __builtin_trap();                                                          \
  }
#define KiloBytes(value) ((value) * 1024)
#define MegaBytes(value) ((KiloBytes(value)) * 1024)
#define GigaBytes(value) ((MegaBytes(value)) * 1024)
#define TeraBytes(value) ((GigaBytes(value)) * 1024)

#define ARR_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))
#define CLAMP(val, min, max)                                                   \
  ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))
// ==================== ENGINE / DLL STUFF ====================
typedef struct {
  bool isInitialized;
  size_t permanentMemorySize;
  size_t transientMemorySize;
  RenderQueue *renderQueue;
  void *permamentMemory;
  void *transientMemory;
  bool reloadDLLHappened;
} GameMemory;

typedef struct {
  Vector2 mousePos;
  // Camera3D camera;
  bool mouseButtons[3]; // Left, middle, right
  bool keys[256];       // Keyboard state
} Input;

#define GAME_UPDATE(name)                                                      \
  void name(GameMemory *gameMemory, Input *input, float frameTime)
typedef GAME_UPDATE(GameUpdate);
static GAME_UPDATE(game_update_stub) {
  (void)gameMemory;
  (void)frameTime;
  (void)input;
}

// ==================== DA functions ====================
DA_DEFINE(arr_uint32_t)
DA_DEFINE_ALLOCATOR(arr_uint32_t, MemoryAllocator)

DA_DEFINE(arr_Matrix)
DA_DEFINE_ALLOCATOR(arr_Matrix, MemoryAllocator)

DA_DEFINE(arr_cstr)
DA_DEFINE_ALLOCATOR(arr_cstr, MemoryAllocator)