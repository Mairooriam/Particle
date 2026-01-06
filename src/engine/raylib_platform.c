
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
#include <vulkan/vulkan_core.h>
#include <winnt.h>
#include "glfw3.h"
#include "utils.h"
#include "vulkan/vulkan.h"
#include "cglm/cglm.h"
// TODO: fix timestep https://gafferongames.com/post/fix_your_timestep/
// TODO: Clean up hotloop code
// TODO: clean up this pile of code

// TODO: make proper utils.h not spread around utls and other files
#define ARR_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))

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
  CopyFile(sourceDLLfilepath, tempDLLfilepath, FALSE);

  result.gameCodeDLL = LoadLibraryA(tempDLLfilepath);
  if (!result.gameCodeDLL) {
    DWORD error = GetLastError();
    LOG("Failed to load DLL %s, error: %lu", tempDLLfilepath, error);
  }

  LOG("Trying to load .dlls");
  if (result.gameCodeDLL) {
    result.update =
        (GameUpdate *)GetProcAddress(result.gameCodeDLL, "game_update");
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

// just example of how to do custom validaiton by docs? is this even needed?
typedef struct {
  PFN_vkCreateInstance real_vkCreateInstance;
  PFN_vkCreateDebugUtilsMessengerEXT createDebugUtils;
  PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugUtils;
} vulkanCtx;
vulkanCtx vCtx = {0};
VkDebugUtilsMessengerEXT debugMessenger;
bool checkValidationLayerSupport(const char **requiredLayers,
                                 uint32_t requiredCount) {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, NULL);

  VkLayerProperties *availableLayers =
      malloc(layerCount * sizeof(VkLayerProperties));
  if (!availableLayers) {
    assert(0 && "lol malloc failed");
    return false;
  }
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

  for (uint32_t i = 0; i < requiredCount; ++i) {
    bool layerFound = false;
    for (uint32_t j = 0; j < layerCount; ++j) {
      if (strcmp(requiredLayers[i], availableLayers[j].layerName) == 0) {
        layerFound = true;
        break;
      }
    }
    if (!layerFound) {
      free(availableLayers);
      return false;
    }
  }

  free(availableLayers);
  return true;
}

int getRequiredExtensions(arr_cstr *extensions, bool enableValidationLayers) {
  if (!extensions)
    return 0;

  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
    nob_da_append(extensions, glfwExtensions[i]);
  }

  if (enableValidationLayers) {
    nob_da_append(extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions->count;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {

  fprintf(stderr, "%s\n", pCallbackData->pMessage);
  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    printf("Hello world this is higer error %s\n", pCallbackData->pMessage);
  }

  return VK_FALSE;
}
VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger) {
  vCtx.createDebugUtils =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          instance, "vkCreateDebugUtilsMessengerEXT");
  if (vCtx.createDebugUtils != NULL) {
    return vCtx.createDebugUtils(instance, pCreateInfo, pAllocator,
                                 pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator) {
  vCtx.destroyDebugUtils =
      (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          instance, "vkDestroyDebugUtilsMessengerEXT");
  if (vCtx.destroyDebugUtils != NULL) {
    vCtx.destroyDebugUtils(instance, debugMessenger, pAllocator);
  }
}
void populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT *createInfo) {
  // TODO: tutorial zero intialized createInfo. if problems do that
  createInfo->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo->messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo->pfnUserCallback = debugCallback;
}

void setupDebugMessenger(VkInstance instance, bool enableValidationLayers) {
  if (!enableValidationLayers)
    return;

  VkDebugUtilsMessengerCreateInfoEXT createInfo = {0};
  populateDebugMessengerCreateInfo(&createInfo);

  if (CreateDebugUtilsMessengerEXT(instance, &createInfo, NULL,
                                   &debugMessenger) != VK_SUCCESS) {
    fprintf(stderr, "Failed to setup debug messenger!\n");
    assert(0 && "Failed to setup debug messenger!");
  }
}

int main(void) {

  vCtx.real_vkCreateInstance =
      (PFN_vkCreateInstance)vkGetInstanceProcAddr(NULL, "vkCreateInstance");
  if (!vCtx.real_vkCreateInstance) {
    fprintf(stderr, "Failed to load vkCreateInstance!\n");
    assert(0 && "failed to load vkCreateInstance");
    exit(1);
  }

  char EXEDirPath[MAX_PATH];
  DWORD SizeOfFilename = GetModuleFileNameA(0, EXEDirPath, sizeof(EXEDirPath));
  (void)SizeOfFilename;

  char sourceDLLfilename[] = "application.dll";
  char sourceDLLfilepath[MAX_PATH];
  char tempDLLfilepath[MAX_PATH];
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

  GameCode code = loadGameCode(sourceDLLfilepath, tempDLLfilepath);
  code.reloadDLLRequested = false;
  code.reloadDLLDelay = 0.0f;

#if INTERNAL_BUILD
  LPVOID baseAddress = (LPVOID)TeraBytes((uint64_t)2);
#else
  LPVOID baseAddress = 0;
#endif

  GameMemory gameMemory = {0};
  gameMemory.permanentMemorySize = MegaBytes(256);
  gameMemory.transientMemorySize = GigaBytes((uint64_t)4);

  uint64_t totalSize =
      gameMemory.permanentMemorySize + gameMemory.transientMemorySize;
  gameMemory.permamentMemory = VirtualAlloc(
      baseAddress, totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (!gameMemory.permamentMemory) {
    LOG("FAILED TO ALLOC PERMANENT MEMORY");
  }
  gameMemory.transientMemory =
      ((uint8_t *)gameMemory.permamentMemory + gameMemory.permanentMemorySize);

  if (!gameMemory.transientMemory) {
    LOG("FAILED TO ALLOC TRANSIENT MEMORY");
  }

  // ==================== INIT WINDOW ====================
  const uint32_t WIDTH = 800;
  const uint32_t HEIGHT = 600;

  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  GLFWwindow *window =
      glfwCreateWindow(WIDTH, HEIGHT, "Vulkan window", NULL, NULL);

  // ==================== INIT VULKAN ====================
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
  VkInstance vkInstance;
  VkApplicationInfo appInfo = {0};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Hello Triangle";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

#ifdef SLOW_CODE_ALLOWED
  const int enableValidationLayers = 1;
#else
  const int enableValidationLayers = 0;
#endif

  const char *validationLayers[] = {"VK_LAYER_KHRONOS_validation"};
  const uint32_t validationLayerCount = ARR_COUNT(validationLayers);
  if (enableValidationLayers &&
      !checkValidationLayerSupport(validationLayers, validationLayerCount)) {
    fprintf(stderr, "Validation layers requested, but not available!\n");
    assert(0 && "Validation layer request but not available!");
    exit(1);
  }

  // ==================== SETUP INFO ====================
  VkInstanceCreateInfo createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {0};
  if (enableValidationLayers) {
    createInfo.enabledLayerCount = (uint32_t)validationLayerCount;
    createInfo.ppEnabledLayerNames = validationLayers;

    populateDebugMessengerCreateInfo(&debugCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
  } else {
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = NULL;
  }

  // ==================== SETUP GLFW EXTENSION WITH VULKAN ====================
  // if using some other windowing u need to use different stuff
  arr_cstr *extensions = arr_cstr_create(16);
  getRequiredExtensions(extensions, enableValidationLayers);
  createInfo.enabledExtensionCount = (uint32_t)extensions->count;
  createInfo.ppEnabledExtensionNames = extensions->items;

  if (vkCreateInstance(&createInfo, NULL, &vkInstance) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create Vulkan instance!\n");
    assert(0 && "Failed to create vulkan instance!\n");
    exit(1);
  }
  free(extensions);

  // ==================== SETUP DEBUG MESSENGER ====================
  VkDebugUtilsMessengerCreateInfoEXT createInfoDbg = {0};
  createInfoDbg.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfoDbg.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfoDbg.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfoDbg.pfnUserCallback = debugCallback;
  createInfoDbg.pUserData = NULL;
  if (CreateDebugUtilsMessengerEXT(vkInstance, &createInfoDbg, NULL,
                                   &debugMessenger) != VK_SUCCESS) {
    fprintf(stderr, "Failed to set up debug messanger!");
    assert(0 && "Failed to setup debug messanger!");
  }

  // ==================== PHYSICAL DEVICES ====================
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(vkInstance, &deviceCount, NULL);
  if (deviceCount == 0) {
    fprintf(stderr, "Failed to find GPUs with Vulkan support!\n");
    assert(0 && "Failed to find gpus with vulkan support!");
  }

  VkPhysicalDevice *devices = malloc(deviceCount * sizeof(VkPhysicalDevice));
  if (!devices) {
    fprintf(stderr, "Failed to allocate physicalDevices\n");
    assert(0 && "failed to allocate physicalDevice");
  }

  vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices);
  free(devices);

  // things that dont go away virtual device
  // swap chain
  // example device
  // https://github.com/KhronosGroup/Vulkan-Samples/blob/main/framework/core/device.h
  // https://github.com/lonelydevil/vulkan-tutorial-C-implementation/blob/main/main.c
  // ==================== MAIN LOOP ====================
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  // ==================== CLEAN UP ====================
  if (enableValidationLayers) {
    DestroyDebugUtilsMessengerEXT(vkInstance, debugMessenger, NULL);
  }

  vkDestroyInstance(vkInstance, NULL);

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}

// ==================== VULKAN HELPERS / VALIDATION ====================

VkResult vkCreateInstance(const VkInstanceCreateInfo *pCreateInfo,
                          const VkAllocationCallbacks *pAllocator,
                          VkInstance *pInstance) {

  printf("hello from debug vkCreateInstance\n");
  if (pCreateInfo == NULL || pInstance == NULL) {
    fprintf(stderr, "Null pointer passed to required parameter!\n");
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  return vCtx.real_vkCreateInstance(pCreateInfo, pAllocator, pInstance);
}
