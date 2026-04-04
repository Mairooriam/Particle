
#include <math.h>
#include "glfw/glfw3.h"
#include "raylib_platfrom.h"
#include "log.h"

#include "vulkanLayer.h"
#include "app/application_types.h"
#include "shared.h"

// TODO: fix timestep https://gafferongames.com/post/fix_your_timestep/
// TODO: Clean up hotloop code
// TODO: clean up this pile of code

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
  // // VERTICAL
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

  // float segmentAmount = 32;
  // float segmentSize = M_PI / segmentAmount;
  // for (size_t i = 0; i < segmentAmount * 2 + 1; i++) {
  //
  //   vertices[vIdx++] =
  //       (Vertex){{cosf(segmentSize * i), sinf(segmentSize * i)}, _YELLOW};
  //   indicies[iIdx++] = i + 1;
  // }

  float vX = 1, vY = 1;
  Vertex vectorVertices[2] = {{{0.0f * step, 0.0f * step}, _BLUE},
                              {{vX * step, vY * step}, _BLUE}};

  uint16_t vectorIndices[2] = {vIdx, vIdx + 1};
  memcpy(&vertices[vIdx], vectorVertices, sizeof(vectorVertices));
  memcpy(&indicies[iIdx], vectorIndices, sizeof(vectorIndices));
  vIdx += 2;
  iIdx += 2;
  printf("%f", x);

  vulkanContext vkCtx = {0};
#ifdef SLOW_CODE_ALLOWED
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
  float lastTime = glfwGetTime();
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    float currentTime = glfwGetTime();
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

    // gameMemory.vertices = vertices;
    // gameMemory.vertexCount = ARR_COUNT(vertices);

    code.update(&gameMemory, &input, deltaTime);

    // vkDrawFrame(&vkCtx, vertices, ARR_COUNT(vertices), ARR_COUNT(indicies),
    //             gameMemory.transforms, gameMemory.instanceColors);
    code.clock++;
    flush_logs();
  }

  vkCleanup(&vkCtx);

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
