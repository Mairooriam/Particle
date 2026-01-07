
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
#include <vulkan/vulkan_core.h>
#include <winnt.h>
#include "utils.h"

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw3native.h>
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
// ==================== VULKAN UTILS ====================
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
  (void)messageType;
  (void)pUserData;
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
// NOTE: this is example rating function. if you intent on using make a proper
// one
int rateDeviceSuitability(VkPhysicalDevice device) {
  VkPhysicalDeviceProperties deviceProperties;
  VkPhysicalDeviceFeatures deviceFeatures;
  vkGetPhysicalDeviceProperties(device, &deviceProperties);
  vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

  int score = 0;
  if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
    score += 1000;
  }
  score += deviceProperties.limits.maxImageDimension2D;

  // if (!deviceFeatures.geometryShader) {
  //   return 0;
  // }
  return score;
}

#define DEFINE_OPTIONAL(Type)                                                  \
  typedef struct {                                                             \
    bool is;                                                                   \
    union {                                                                    \
      Type val;                                                                \
    };                                                                         \
  } optional_##Type
DEFINE_OPTIONAL(uint32_t);

typedef struct {
  optional_uint32_t graphicsFamily;
  optional_uint32_t presentFamily;
  bool isComplete;
  uint32_t queueUniqueFamilies[2];
  uint32_t queueUniqueFamiliesCount;
} QueueFamilyIndices;

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device,
                                     VkSurfaceKHR surface) {
  QueueFamilyIndices indicies = {0};
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);
  VkQueueFamilyProperties *queueFamilies =
      malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           queueFamilies);
  for (size_t i = 0; i < queueFamilyCount; i++) {
    if (indicies.graphicsFamily.is) {
      break;
    }

    // ==================== GRAPGICS FAMILY ====================
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indicies.graphicsFamily.val = i;
      indicies.graphicsFamily.is = true;
    }

    // ==================== SURFACE SUPPORT ====================
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
    if (presentSupport && !indicies.presentFamily.is) {
      indicies.presentFamily.val = i;
      indicies.presentFamily.is = true;
    }
  }

  if (!indicies.graphicsFamily.is) {
    indicies.graphicsFamily.val = UINT32_MAX;
  }

  if (!indicies.presentFamily.is) {
    indicies.presentFamily.val = UINT32_MAX;
  }

  indicies.queueUniqueFamiliesCount = 0;
  if (indicies.graphicsFamily.is && indicies.presentFamily.is) {
    indicies.isComplete = true;
    if (indicies.graphicsFamily.val != indicies.presentFamily.val) {
      indicies.queueUniqueFamilies[indicies.queueUniqueFamiliesCount++] =
          indicies.graphicsFamily.val;
      indicies.queueUniqueFamilies[indicies.queueUniqueFamiliesCount++] =
          indicies.presentFamily.val;
    } else {
      indicies.queueUniqueFamilies[indicies.queueUniqueFamiliesCount++] =
          indicies.graphicsFamily.val;
    }
  }

  return indicies;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device,
                                 const char **requiredExtensions,
                                 uint32_t requiredCount) {
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);

  VkExtensionProperties *availableExtensions =
      malloc(extensionCount * sizeof(VkExtensionProperties));
  if (!availableExtensions) {
    return false;
  }
  vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount,
                                       availableExtensions);

  for (uint32_t i = 0; i < requiredCount; ++i) {
    bool extensionFound = false;
    for (uint32_t j = 0; j < extensionCount; ++j) {
      if (strcmp(requiredExtensions[i], availableExtensions[j].extensionName) ==
          0) {
        extensionFound = true;
        break;
      }
    }
    if (!extensionFound) {
      free(availableExtensions);
      return false;
    }
  }

  free(availableExtensions);
  return true;
}
typedef struct {
  VkSurfaceCapabilitiesKHR capabilities;
  VkSurfaceFormatKHR *formats;
  uint32_t formatsCount;
  VkPresentModeKHR *presentModes;
  uint32_t presentModesCount;
} SwapChainSupportDetails;
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device,
                                              VkSurfaceKHR surface) {
  SwapChainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                            &details.capabilities);
  // ==================== FORMATS ====================
  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, NULL);
  if (formatCount != 0) {
    details.formats = malloc(sizeof(VkSurfaceFormatKHR) * formatCount);
    details.formatsCount = formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                         details.formats);
  } else {
    details.formatsCount = 0;
  }
  // ==================== PRESENT MODES ====================
  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount,
                                            NULL);
  if (presentModeCount != 0) {
    details.presentModes = malloc(sizeof(VkPresentModeKHR) * presentModeCount);
    details.presentModesCount = presentModeCount;
  } else {
    details.presentModesCount = 0;
  }

  return details;
}
bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface,
                      const char **requiredExtensions,
                      size_t requiredExtensionsCount) {
  QueueFamilyIndices indicies = findQueueFamilies(device, surface);
  bool extensionsSupported = checkDeviceExtensionSupport(
      device, requiredExtensions, requiredExtensionsCount);
  bool swapChainAdequate = false;
  if (extensionsSupported) {
    SwapChainSupportDetails swapChainSupport =
        querySwapChainSupport(device, surface);
    swapChainAdequate = swapChainSupport.formatsCount != 0 &&
                        swapChainSupport.presentModesCount != 0;
  }

  return indicies.isComplete && extensionsSupported && swapChainAdequate;
}
VkSurfaceFormatKHR
chooseSwapSurfaceFormat(const VkSurfaceFormatKHR *availableFormats,
                        uint32_t availableFormatsCount) {
  for (size_t i = 0; i < availableFormatsCount; i++) {
    if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormats[i];
    }
  }
  return availableFormats[0];
}
VkPresentModeKHR
chooseSwapPresentMode(const VkPresentModeKHR *availablePresentModes,
                      uint32_t availablePresentModescount) {
  for (size_t i = 0; i < availablePresentModescount; i++) {
    if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      return availablePresentModes[i];
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

#define CLAMP(val, min, max)                                                   \
  ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR *capabilities,
                            GLFWwindow *window) {
  if (capabilities->currentExtent.width != UINT32_MAX) {
    return capabilities->currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = {(uint32_t)(width), (uint32_t)(height)};

    actualExtent.width =
        CLAMP(actualExtent.width, capabilities->minImageExtent.width,
              capabilities->maxImageExtent.width);
    actualExtent.height =
        CLAMP(actualExtent.height, capabilities->minImageExtent.height,
              capabilities->maxImageExtent.height);

    return actualExtent;
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

  const char *requiredExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  const uint32_t requiredExtensionsCount = ARR_COUNT(requiredExtensions);

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
  // ==================== SETUP SURFACE ====================
  VkSurfaceKHR surface;
  VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {0};
  surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  surfaceCreateInfo.hwnd = glfwGetWin32Window(window);
  surfaceCreateInfo.hinstance = GetModuleHandle(NULL);

  if (vkCreateWin32SurfaceKHR(vkInstance, &surfaceCreateInfo, NULL, &surface) !=
      VK_SUCCESS) {
    fprintf(stderr, "failed to create window surface!");
  }
  // ==================== PICK PHYSICAL DEVICE ====================
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  uint32_t deviceCount = 0;
  VkResult enumResult =
      vkEnumeratePhysicalDevices(vkInstance, &deviceCount, NULL);
  if (enumResult != VK_SUCCESS || deviceCount == 0) {
    fprintf(stderr,
            "Failed to find GPUs with Vulkan support! Result: %d, Count: %u\n",
            enumResult, deviceCount);
    assert(0 && "Failed to find GPUs with Vulkan support!");
  }

  VkPhysicalDevice *devices = malloc(deviceCount * sizeof(VkPhysicalDevice));
  if (!devices) {
    fprintf(stderr, "Failed to allocate memory for physical devices!\n");
    assert(0 && "Failed to allocate memory for physical devices!");
  }

  enumResult = vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices);
  if (enumResult != VK_SUCCESS) {
    fprintf(stderr, "Failed to enumerate physical devices! Result: %d\n",
            enumResult);
    free(devices);
    assert(0 && "Failed to enumerate physical devices!");
  }
  int bestScore = -1;
  for (size_t i = 0; i < deviceCount; i++) {
    if (!isDeviceSuitable(devices[i], surface, requiredExtensions,
                          requiredExtensionsCount)) {
      continue;
    }

    int score = rateDeviceSuitability(devices[i]);
    if (score > bestScore) {
      bestScore = score;
      physicalDevice = devices[i];
    }
  }

  if (bestScore <= 0 || physicalDevice == VK_NULL_HANDLE) {
    fprintf(stderr, "Faiiled to find suitable GPU!\n");
    assert(0 && "failed to find suitable GPU!\n");
  }

  free(devices);

  // ==================== LOGICAL DEVICE ====================
  VkDevice logicalDevice;

  QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

  float queuePriority = 1.0f;
  VkDeviceQueueCreateInfo queueCreateInfos[indices.queueUniqueFamiliesCount];
  for (size_t i = 0; i < indices.queueUniqueFamiliesCount; i++) {
    VkDeviceQueueCreateInfo queueCreateInfo = {0};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = indices.queueUniqueFamilies[i];
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos[i] = queueCreateInfo;
  }
  // We'll come back to this structure once we're about to start doing
  // more interesting things with Vulkan.
  VkPhysicalDeviceFeatures deviceFeatures = {0};
  VkDeviceCreateInfo LogigalCreateInfo = {0};
  LogigalCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  LogigalCreateInfo.pQueueCreateInfos = queueCreateInfos;
  LogigalCreateInfo.queueCreateInfoCount = indices.queueUniqueFamiliesCount;
  LogigalCreateInfo.pEnabledFeatures = &deviceFeatures;
  LogigalCreateInfo.enabledExtensionCount = requiredExtensionsCount;
  LogigalCreateInfo.ppEnabledExtensionNames = requiredExtensions;

  if (enableValidationLayers) {
    LogigalCreateInfo.enabledLayerCount = validationLayerCount;
    LogigalCreateInfo.ppEnabledLayerNames = validationLayers;
  } else {
    LogigalCreateInfo.enabledLayerCount = 0;
  }

  if (vkCreateDevice(physicalDevice, &LogigalCreateInfo, NULL,
                     &logicalDevice) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create logical device!");
    assert(0 && "Failed to create logical device");
  }

  VkQueue presentQueue;
  vkGetDeviceQueue(logicalDevice, indices.presentFamily.val, 0, &presentQueue);
  VkQueue graphicsQueue;
  vkGetDeviceQueue(logicalDevice, indices.graphicsFamily.val, 0,
                   &graphicsQueue);

  // ==================== SWAP CHAIN ====================
  SwapChainSupportDetails swapChainSupport =
      querySwapChainSupport(physicalDevice, surface);

  VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(
      swapChainSupport.formats, swapChainSupport.formatsCount);
  VkPresentModeKHR presentMode = chooseSwapPresentMode(
      swapChainSupport.presentModes, swapChainSupport.presentModesCount);
  VkExtent2D extent = chooseSwapExtent(&swapChainSupport.capabilities, window);
  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

  if (swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR swapChainCreateInfo = {0};
  swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapChainCreateInfo.surface = surface;
  swapChainCreateInfo.minImageCount = imageCount;
  swapChainCreateInfo.imageFormat = surfaceFormat.format;
  swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
  swapChainCreateInfo.imageExtent = extent;
  swapChainCreateInfo.imageArrayLayers = 1;
  swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  uint32_t queueFamilyIndices[] = {indices.graphicsFamily.val,
                                   indices.presentFamily.val};
  if (indices.graphicsFamily.val != indices.presentFamily.val) {
    swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapChainCreateInfo.queueFamilyIndexCount = 2;
    swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainCreateInfo.queueFamilyIndexCount = 0;
    swapChainCreateInfo.pQueueFamilyIndices = NULL;
  }

  swapChainCreateInfo.preTransform =
      swapChainSupport.capabilities.currentTransform;
  swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapChainCreateInfo.presentMode = presentMode;
  swapChainCreateInfo.clipped = VK_TRUE;
  swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

  VkSwapchainKHR swapChain;
  if (vkCreateSwapchainKHR(logicalDevice, &swapChainCreateInfo, NULL,
                           &swapChain) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create swap chain!\n");
    assert(0 && "Failed to create swap chain!");
  }

  vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, NULL);
  VkImage *swapChainImages = malloc(sizeof(VkImage) * imageCount);
  if (!swapChainImages) {
    assert(0 && "Lol malloc failed gl");
  }
  vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount,
                          swapChainImages);

  // TODO: add to ctx when time for that
  VkFormat swapChainImageFormat = surfaceFormat.format;
  VkExtent2D swapChainExtent = extent;

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
  vkDestroySwapchainKHR(logicalDevice, swapChain, NULL);
  vkDestroySurfaceKHR(vkInstance, surface, NULL);
  vkDestroyDevice(logicalDevice, NULL);
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
