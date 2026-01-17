
#include <math.h>
#include <raylib.h>
#include "fix_win32_compatibility.h"
#include "rlgl.h"
#include "raymath.h"
#include "raylib_platfrom.h"

#include "log.h"
#include "shared.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysinfoapi.h>
#include <winnt.h>
#include "utils.h"

#include "vulkanLayer.h"
#include "app/application_types.h"

// TODO: fix timestep https://gafferongames.com/post/fix_your_timestep/
// TODO: Clean up hotloop code
// TODO: clean up this pile of code

static FILETIME getFileLastWriteTime(const char *filename) {
  FILETIME result;
  WIN32_FILE_ATTRIBUTE_DATA fileInfo;
  if (GetFileAttributesExA(filename, GetFileExInfoStandard, &fileInfo)) {
    result = fileInfo.ftLastWriteTime;
  } else {
    result = (FILETIME){0};
  }
  return result;
}

// source: https://guide.handmadehero.org/code/day021/
static GameCode loadGameCode(char *sourceDLLfilepath, char *tempDLLfilepath) {
  set_log_prefix("[loadGameCode] ");
  GameCode result = {0};
  result.currentDLLtimestamp = getFileLastWriteTime(sourceDLLfilepath);
  DeleteFileA(tempDLLfilepath);
  if (!CopyFile(sourceDLLfilepath, tempDLLfilepath, FALSE)) {
    DWORD error = GetLastError();
    LOG("Failed to copy DLL from %s to %s. Error: %lu", sourceDLLfilepath,
        tempDLLfilepath, error);
    result.isvalid = false;
    result.update = game_update_stub;
    return result;
  }

  result.gameCodeDLL = LoadLibraryA(tempDLLfilepath);
  if (!result.gameCodeDLL) {
    DWORD error = GetLastError();
    LOG("Failed to load DLL %s, error: %lu", tempDLLfilepath, error);
  }

  LOG("Trying to load .dlls");
  if (result.gameCodeDLL) {
    result.update =
        (GameUpdate *)(void *)GetProcAddress(result.gameCodeDLL, "game_update");
    if (result.update) {
      result.isvalid = true;
      LOG("Loading .dlls was succesfull");
    } else {
      LOG("Failed to get function address for game_update");
    }
  }

  if (!result.isvalid) {
    result.update = game_update_stub;
    LOG("Loading .dlls wasn't succesfull. Resetting to stub functions. "
        "DLL_SOURCE: %s , DLL_TEMP: %s",
        sourceDLLfilepath, tempDLLfilepath);
  }

  return result;
}
static void unloadGameCode(GameCode *gameCode) {
  set_log_prefix("[unloadGameCode]");
  if (gameCode->gameCodeDLL) {
    FreeLibrary(gameCode->gameCodeDLL);
    LOG("Freed .dlls");
  }
  gameCode->isvalid = false;
  gameCode->update = game_update_stub;
}

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

void saveGameMemory(const char *filePath, GameMemory *gameMemory) {
  HANDLE file = CreateFileA(filePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL, NULL);
  Assert(file != INVALID_HANDLE_VALUE);

  DWORD bytesWritten;
  BOOL result = WriteFile(file, gameMemory->permamentMemory,
                          gameMemory->permanentMemorySize, &bytesWritten, NULL);
  // Optionally check if bytesWritten == gameMemory->permanentMemorySize
  Assert(
      result !=
      0) // Will break stuff down the line. add handling if u come accros this.
      CloseHandle(file);
}

void loadGameMemory(const char *filePath, GameMemory *gameMemory) {
  HANDLE file = CreateFileA(filePath, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL, NULL);
  Assert(file != INVALID_HANDLE_VALUE);

  DWORD bytesRead;
  BOOL result = ReadFile(file, gameMemory->permamentMemory,
                         gameMemory->permanentMemorySize, &bytesRead, NULL);
  Assert(
      result !=
      0); // Will break stuff down the line. add handling if u come accros this.
  Assert(bytesRead == gameMemory->permanentMemorySize);
  CloseHandle(file);
}

// TODO: use something other than raylib?
static void collect_input(Input *input) {
  input->mousePos = GetMousePosition();
  for (int i = 0; i < 3; i++) {
    input->mouseButtons[i] = IsMouseButtonDown(i);
  }
  for (int i = 0; i < 256; i++) {
    input->keys[i] = IsKeyDown(i);
  }
}

// static void appendToFile(FILE *h, void *base, size_t size) {
//   BOOL result = WriteFile(h, base, size, NULL, NULL);
// }

static void save_input_to_file(HANDLE h, Input *input) {
  DWORD written;
  BOOL result = WriteFile(h, input, sizeof(Input), &written, NULL);
  Assert(written == sizeof(Input));
  // Will break stuff down the line. add handling if u come accros this.
  Assert(result != 0);
}

void window_size_callback(GLFWwindow *window, int width, int height) {
  (void)width;
  (void)height;
  printf("hello from callback\n");
  vulkanContext *app = (vulkanContext *)(glfwGetWindowUserPointer(window));
  app->framebufferResized = true;
}
int main(void) {
#ifdef __cplusplus
#warning "This file is being compiled as C++"
  printf("this is c++")
#else
#warning "This file is being compiled as C"
  printf("this is C");
#endif
      char EXEDirPath[MAX_PATH];
  DWORD SizeOfFilename = GetModuleFileNameA(0, EXEDirPath, sizeof(EXEDirPath));
  (void)SizeOfFilename;

  char sourceDLLfilename[] = "application.dll";
  char sourceDLLfilepath[MAX_PATH];
  char tempDLLfilepath[MAX_PATH];
  char shaderDir[] = "/shader";
  char shaderPath[MAX_PATH];
  char tempDLLfilename[] = "libapplication_temp.dll";

  char *onePastLastSlash = EXEDirPath;
  for (char *scan = EXEDirPath; *scan; ++scan) {
    if (*scan == '\\') {
      onePastLastSlash = scan + 1;
    }
  }

  ConcatStrings(onePastLastSlash - EXEDirPath, EXEDirPath,
                sizeof(sourceDLLfilename) - 1, sourceDLLfilename,
                sizeof(sourceDLLfilepath), sourceDLLfilepath);

  ConcatStrings(onePastLastSlash - EXEDirPath, EXEDirPath,
                sizeof(tempDLLfilename) - 1, tempDLLfilename,
                sizeof(tempDLLfilepath), tempDLLfilepath);
  ConcatStrings(onePastLastSlash - EXEDirPath, EXEDirPath, sizeof(shaderDir),
                shaderDir, sizeof(shaderPath), shaderPath);

  GameCode code = loadGameCode(sourceDLLfilepath, tempDLLfilepath);
  code.reloadDLLRequested = false;
  code.reloadDLLDelay = 0.1f;

#if INTERNAL_BUILD
  LPVOID baseAddress = (LPVOID)TeraBytes((uint64_t)2);
#else
  LPVOID baseAddress = 0;
#endif

  GameMemory gameMemory = {0};
  gameMemory.permanentMemorySize = MegaBytes(512);
  gameMemory.transientMemorySize = MegaBytes(512);
  gameMemory.reloadDLLHappened = false;

  uint64_t totalSize =
      gameMemory.permanentMemorySize + gameMemory.transientMemorySize;
  gameMemory.permamentMemory = VirtualAlloc(
      baseAddress, totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (!gameMemory.permamentMemory) {
    LOG("FAILED TO ALLOC PERMANENT MEMORY");
    assert(0 && "lol alloc failed");
  }
  gameMemory.transientMemory =
      ((uint8_t *)gameMemory.permamentMemory + gameMemory.permanentMemorySize);

  if (!gameMemory.transientMemory) {
    LOG("FAILED TO ALLOC TRANSIENT MEMORY");
    assert(0 && "Lmao alloc failed");
  }
  LOG("Allocated memory:");
  printf("  Permanent Memory: %p (Size: %llu)\n", gameMemory.permamentMemory,
         gameMemory.permanentMemorySize);
  printf("  Transient Memory: %p (Size: %llu)\n", gameMemory.transientMemory,
         gameMemory.transientMemorySize);
  // ==================== INIT WINDOW ====================
  const uint32_t WIDTH = 800;
  const uint32_t HEIGHT = 600;

  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  GLFWwindow *window =
      glfwCreateWindow(WIDTH, HEIGHT, "Vulkan window", NULL, NULL);
  glfwSetWindowSizeCallback(window, window_size_callback);

// TODO: make it proper just placeholder currently
#define _RED {1.0f, 0.0f, 0.0f}
#define _GREEN {0.0f, 1.0f, 0.0f}
#define _BLUE {0.0f, 0.0f, 1.0f}
#define _YELLOW {1.0f, 1.0f, 0.0f}
  //
  // Vertex vertices[] = {
  //     {{-0.5f, -0.5f}, _RED}, // Bottom-left
  //     {{0.5f, -0.5f}, _GREEN},
  //     {{0.5f, 0.5f}, _BLUE},    // Top-right
  //     {{-0.5f, 0.5f}, _YELLOW}, // Top-left
  // };

  float step = 0.1f;
  int numLines = (int)((2.0f / step) + 1);
  int count = numLines * 2;
  printf("%i\n", count);
  // -1 <--> 1
  Vertex vertices[count * 2 + 2];
  uint16_t indicies[count * 2 + 2];

  float x = -1, y = -1, z = 0;
  int vIdx = 0, iIdx = 0;
  // VERTICAL
  for (int i = 0; i < count; i++) {
    if (i % 2 == 0) {
      y = -1;
      x += step;
    } else {
      y = 1;
    }

    if (i == 0) {
      x = -1;
    }
    vertices[vIdx++] = (Vertex){{x, y, z}, _RED};
    indicies[iIdx++] = i;
  }

  x = 1, y = -1, z = 0;
  // HORIZONTAL
  for (int i = 0; i < count; i++) {
    if (i % 2 == 0) {
      x = -1;
      y += step;
    } else {
      x = 1;
    }

    if (i == 0) {
      y = -1;
    }

    vertices[vIdx++] = (Vertex){{x, y, z}, _RED};
    indicies[iIdx++] = count + i;
  }

  float vX = 1, vY = 2;
  Vertex vectorVertices[2] = {{{0.0f * step, 0.0f * step}, _BLUE},
                              {{vX * step, vY * step}, _BLUE}};

  uint16_t vectorIndices[2] = {vIdx, vIdx + 1};
  memcpy(&vertices[vIdx], vectorVertices, sizeof(vectorVertices));
  memcpy(&indicies[iIdx], vectorIndices, sizeof(vectorIndices));
  vIdx += 2;
  iIdx += 2;
  printf("%f", x);

  vulkanContext vkCtx = {0};
#if defined(SLOW_CODE_ALLOWED)
  g_real_vkCreateInstance =
      (PFN_vkCreateInstance)vkGetInstanceProcAddr(NULL, "vkCreateInstance");
#endif

  vkInit(&vkCtx, window, vertices, ARR_COUNT(vertices), indicies,
         ARR_COUNT(indicies));

  // things that dont go away virtual device
  // swap chain
  // example device
  // https://github.com/KhronosGroup/Vulkan-Samples/blob/main/framework/core/device.h
  // https://github.com/lonelydevil/vulkan-tutorial-C-implementation/blob/main/main.c
  // ==================== MAIN LOOP ====================
  Input input = {0};
  float lastTime = GetTime();
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    float currentTime = GetTime();
    float deltaTime = currentTime - lastTime; // Delta time in seconds
    lastTime = currentTime;
    // ==================== HOT RELOADING ====================
    if (code.reloadDLLRequested && (code.clock >= code.reloadDLLDelay)) {
      LOG("Reloading DLLs.");
      unloadGameCode(&code);
      code = loadGameCode(sourceDLLfilepath, tempDLLfilepath);
      code.reloadDLLRequested = false;
      code.clock = 0;
      gameMemory.reloadDLLHappened = true;
    }

    FILETIME time = getFileLastWriteTime(sourceDLLfilepath);
    if (CompareFileTime(&time, &code.currentDLLtimestamp) != 0) {
      code.reloadDLLRequested = true;
    }

    gameMemory.vertices = vertices;
    gameMemory.vertexCount = ARR_COUNT(vertices);

    code.update(&gameMemory, &input, deltaTime);

    vkDrawFrame(&vkCtx, vertices, ARR_COUNT(vertices), ARR_COUNT(indicies));
    code.clock++;
    flush_logs();
  }

  vkCleanup(&vkCtx);

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}

// ==================== VULKAN HELPERS / VALIDATION ====================

// TODO: setup this up?
