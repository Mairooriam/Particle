#pragma once
#include "internal/renderQue.h"
#include "shader.h"

#ifdef _WIN32
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

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
  ShaderData *shaderData;
  void *permamentMemory;
  void *transientMemory;
  bool reloadDLLHappened;
} GameMemory;

typedef struct Input {
  bool quit;
  bool keys[512]; // indexed by SDL_Scancode
  bool mouseButtons[5];
  float mouseX, mouseY;
  float mouseDeltaX, mouseDeltaY;
  float mouseWheel;
} Input;
#define GAME_UPDATE(name)                                                      \
  void name(GameMemory *gameMemory, Input *input, float frameTime)
typedef GAME_UPDATE(GameUpdate);
static GAME_UPDATE(game_update_stub) {
  (void)gameMemory;
  (void)frameTime;
  (void)input;
}
