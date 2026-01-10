
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

typedef struct {
  uint32_t *code;
  size_t size;
} ShaderCode;

size_t getFileSize(const char *filename) {
  WIN32_FILE_ATTRIBUTE_DATA fileInfo;
  if (!GetFileAttributesExA(filename, GetFileExInfoStandard, &fileInfo)) {
    fprintf(stderr, "Failed to get file attributes for: %s\n", filename);
    return 0;
  }

  LARGE_INTEGER size;
  size.HighPart = fileInfo.nFileSizeHigh;
  size.LowPart = fileInfo.nFileSizeLow;

  return (size_t)size.QuadPart;
}

int readShaderFile(const char *filename, ShaderCode *shader) {
  HANDLE file = CreateFileA(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL, NULL);
  if (file == INVALID_HANDLE_VALUE) {
    fprintf(stderr, "Failed to open shader file: %s\n", filename);
    return 0;
  }

  DWORD bytesRead;
  BOOL result =
      ReadFile(file, shader->code, (DWORD)shader->size, &bytesRead, NULL);
  if (!result || bytesRead != shader->size) {
    fprintf(stderr, "Failed to read shader file completely\n");
    shader->code = NULL;
    CloseHandle(file);
    return 0;
  }

  CloseHandle(file);
  return 1;
}

VkShaderModule createShaderModule(const ShaderCode *shader, VkDevice device) {
  VkShaderModuleCreateInfo createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = shader->size;
  createInfo.pCode = shader->code;

  VkShaderModule shaderModule = {0};
  if (vkCreateShaderModule(device, &createInfo, NULL, &shaderModule) !=
      VK_SUCCESS) {
    fprintf(stderr, "failed to create shader module!");
    assert(0 && "failed to create shader module!");
  }
  return shaderModule;
}

void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex,
                         VkRenderPass renderPass,
                         VkFramebuffer *swapChainFramebuffers,
                         VkExtent2D swapChainExtent,
                         VkPipeline graphicsPipeline) {
  assert(swapChainFramebuffers &&
         "Dont call record command buffer without frambuffer");
  VkCommandBufferBeginInfo beginInfo = {0};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;               // Optional
  beginInfo.pInheritanceInfo = NULL; // Optional

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    fprintf(stderr, "failed to begin recording command buffer!");
    assert(0 && "failed to begin recording command buffer!");
  }

  VkRenderPassBeginInfo renderPassInfo = {0};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
  renderPassInfo.renderArea.offset = (VkOffset2D){0, 0};
  renderPassInfo.renderArea.extent = swapChainExtent;

  VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    graphicsPipeline);

  VkViewport viewport = {0};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = swapChainExtent.width;
  viewport.height = swapChainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor = {0};
  scissor.offset = (VkOffset2D){0, 0};
  scissor.extent = swapChainExtent;
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
  vkCmdDraw(commandBuffer, 3, 1, 0, 0);
  vkCmdEndRenderPass(commandBuffer);
  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    fprintf(stderr, "failed to reconrd command buffer!");
    assert(0 && "failed to create command buffer!");
  }
}

// void drawFrame(VkDevice device, const VkFence *inFlightFence, ) {
//  }

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

  QueueFamilyIndices LogicalDeviceIndices =
      findQueueFamilies(physicalDevice, surface);

  float queuePriority = 1.0f;
  VkDeviceQueueCreateInfo
      queueCreateInfos[LogicalDeviceIndices.queueUniqueFamiliesCount];
  for (size_t i = 0; i < LogicalDeviceIndices.queueUniqueFamiliesCount; i++) {
    VkDeviceQueueCreateInfo queueCreateInfo = {0};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex =
        LogicalDeviceIndices.queueUniqueFamilies[i];
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
  LogigalCreateInfo.queueCreateInfoCount =
      LogicalDeviceIndices.queueUniqueFamiliesCount;
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
  vkGetDeviceQueue(logicalDevice, LogicalDeviceIndices.presentFamily.val, 0,
                   &presentQueue);
  VkQueue graphicsQueue;
  vkGetDeviceQueue(logicalDevice, LogicalDeviceIndices.graphicsFamily.val, 0,
                   &graphicsQueue);

  // ==================== SWAP CHAIN ====================
  SwapChainSupportDetails swapChainSupport =
      querySwapChainSupport(physicalDevice, surface);

  // CHOOSE SWAP SURFACE FORMAT
  VkSurfaceFormatKHR swapChainSurfaceFormat;
  for (size_t i = 0; i < swapChainSupport.formatsCount; i++) {
    if (swapChainSupport.formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
        swapChainSupport.formats[i].colorSpace ==
            VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      swapChainSurfaceFormat = swapChainSupport.formats[i];
    }
  }
  swapChainSurfaceFormat = swapChainSupport.formats[0];

  VkPresentModeKHR presentMode = chooseSwapPresentMode(
      swapChainSupport.presentModes, swapChainSupport.presentModesCount);
  VkExtent2D swapChainExtent =
      chooseSwapExtent(&swapChainSupport.capabilities, window);
  uint32_t swapChainImageCount =
      swapChainSupport.capabilities.minImageCount + 1;

  if (swapChainSupport.capabilities.maxImageCount > 0 &&
      swapChainImageCount > swapChainSupport.capabilities.maxImageCount) {
    swapChainImageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR swapChainCreateInfo = {0};
  swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapChainCreateInfo.surface = surface;
  swapChainCreateInfo.minImageCount = swapChainImageCount;
  swapChainCreateInfo.imageFormat = swapChainSurfaceFormat.format;
  swapChainCreateInfo.imageColorSpace = swapChainSurfaceFormat.colorSpace;
  swapChainCreateInfo.imageExtent = swapChainExtent;
  swapChainCreateInfo.imageArrayLayers = 1;
  swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  uint32_t queueFamilyIndices[] = {LogicalDeviceIndices.graphicsFamily.val,
                                   LogicalDeviceIndices.presentFamily.val};
  if (LogicalDeviceIndices.graphicsFamily.val !=
      LogicalDeviceIndices.presentFamily.val) {
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

  vkGetSwapchainImagesKHR(logicalDevice, swapChain, &swapChainImageCount, NULL);
  VkImage *swapChainImages = malloc(sizeof(VkImage) * swapChainImageCount);
  if (!swapChainImages) {
    assert(0 && "Lol malloc failed gl");
  }
  vkGetSwapchainImagesKHR(logicalDevice, swapChain, &swapChainImageCount,
                          swapChainImages);

  // ==================== IMAGE VIEWS ====================
  VkImageView *swapChainImageViews =
      malloc(sizeof(VkImageView) * swapChainImageCount);

  for (size_t i = 0; i < swapChainImageCount; i++) {
    VkImageViewCreateInfo ImageViewCreateInfo = {0};
    ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImageViewCreateInfo.image = swapChainImages[i];
    ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ImageViewCreateInfo.format = swapChainSurfaceFormat.format;
    ImageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    ImageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    ImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    ImageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    ImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    ImageViewCreateInfo.subresourceRange.levelCount = 1;
    ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    ImageViewCreateInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(logicalDevice, &ImageViewCreateInfo, NULL,
                          &swapChainImageViews[i]) != VK_SUCCESS) {
      fprintf(stderr, "failed to create image views!");
      assert(0 && "failed to create image views!");
    }
  }

  free(swapChainImages);

  // ==================== GRAPGICS PIPELINE - SHADERS ====================
  size_t shadersize = getFileSize("shader/vert.spv");
  assert(shadersize > 0 && "Is the path valid?");
  ShaderCode vertShaderCode = {0};
  vertShaderCode.code = malloc(shadersize);
  vertShaderCode.size = shadersize;
  shadersize = 0;

  readShaderFile("shader/vert.spv", &vertShaderCode);

  shadersize = getFileSize("shader/frag.spv");
  assert(shadersize > 0 && "Is the path valid?");
  ShaderCode fragShaderCode = {0};
  fragShaderCode.code = malloc(shadersize);
  fragShaderCode.size = shadersize;

  readShaderFile("shader/frag.spv", &fragShaderCode);

  VkShaderModule vertShaderModule =
      createShaderModule(&vertShaderCode, logicalDevice);
  VkShaderModule fragShaderModule =
      createShaderModule(&fragShaderCode, logicalDevice);

  free(vertShaderCode.code);
  free(fragShaderCode.code);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {0};
  vertShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {0};
  fragShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                    fragShaderStageInfo};

  VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                    VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamicState = {0};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = ARR_COUNT(dynamicStates);
  dynamicState.pDynamicStates = dynamicStates;

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.pVertexBindingDescriptions = NULL;
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  vertexInputInfo.pVertexAttributeDescriptions = NULL;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {0};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)swapChainExtent.width;
  viewport.height = (float)swapChainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {0};
  scissor.offset = (VkOffset2D){0, 0};
  scissor.extent = swapChainExtent;

  VkPipelineViewportStateCreateInfo viewportState = {0};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  // ==================== RASTERIZER ====================
  VkPipelineRasterizationStateCreateInfo rasterizer = {0};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f; // Optional
  rasterizer.depthBiasClamp = 0.0f;          // Optional
  rasterizer.depthBiasSlopeFactor = 0.0f;    // Optional

  // ==================== MULTISAMPLING ====================
  VkPipelineMultisampleStateCreateInfo multisampling = {0};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f;          // Optional
  multisampling.pSampleMask = NULL;               // Optional
  multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
  multisampling.alphaToOneEnable = VK_FALSE;      // Optional

  // ==================== COLOR BLENDING ====================
  VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;             // Optional
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;             // Optional

  VkPipelineColorBlendStateCreateInfo colorBlending = {0};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f; // Optional
  colorBlending.blendConstants[1] = 0.0f; // Optional
  colorBlending.blendConstants[2] = 0.0f; // Optional
  colorBlending.blendConstants[3] = 0.0f; // Optional

  VkPipelineLayout pipelineLayout = {0};
  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;         // Optional
  pipelineLayoutInfo.pSetLayouts = NULL;         // Optional
  pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
  pipelineLayoutInfo.pPushConstantRanges = NULL; // Optional

  if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, NULL,
                             &pipelineLayout) != VK_SUCCESS) {
    fprintf(stderr, "failed to create pipeline layout!");
    assert(0 && "failed to create pipeline layout!");
  }

  // ==================== RENDER PASSES ====================
  VkAttachmentDescription colorAttachment = {0};
  colorAttachment.format = swapChainSurfaceFormat.format;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef = {0};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  VkSubpassDescription subpass = {0};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkSubpassDependency dependency = {0};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPass renderPass;
  VkRenderPassCreateInfo renderPassInfo = {0};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (vkCreateRenderPass(logicalDevice, &renderPassInfo, NULL, &renderPass) !=
      VK_SUCCESS) {
    fprintf(stderr, "failed to create render pass!");
    assert(0 && "failed to create render pass!");
  }

  VkGraphicsPipelineCreateInfo pipelineInfo = {0};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = NULL; // Optional
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass =
      0; // https://docs.vulkan.org/spec/latest/chapters/renderpass.html#renderpass-compatibility
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
  pipelineInfo.basePipelineIndex = -1;              // Optional

  VkPipeline graphicsPipeline;
  if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo,
                                NULL, &graphicsPipeline) != VK_SUCCESS) {
    fprintf(stderr, "failed to create graphics pipeline!");
    assert(0 && "failed to create graphics pipeline!");
  }

  // ==================== FRAMEBUFFERS ====================

  VkFramebuffer *swapChainFramebuffers =
      malloc(sizeof(VkFramebuffer) * swapChainImageCount);
  for (size_t i = 0; i < swapChainImageCount; i++) {
    VkImageView attachments[] = {swapChainImageViews[i]};

    VkFramebufferCreateInfo framebufferInfo = {0};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = swapChainExtent.width;
    framebufferInfo.height = swapChainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, NULL,
                            &swapChainFramebuffers[i]) != VK_SUCCESS) {
      fprintf(stderr, "failed to create framebuffer");
      assert(0 && "failed to create framebuffer!");
    }
  }

  // ==================== COMMAND POOLS ====================

  VkCommandPool commandPool;
  VkCommandPoolCreateInfo poolInfo = {0};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = LogicalDeviceIndices.graphicsFamily.val;

  if (vkCreateCommandPool(logicalDevice, &poolInfo, NULL, &commandPool) !=
      VK_SUCCESS) {
    fprintf(stderr, "failed to create command pool!");
    assert(0 && "failed to create command pool!");
  }

  VkCommandBuffer commandBuffer;
  VkCommandBufferAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer) !=
      VK_SUCCESS) {
    fprintf(stderr, "failed to allocate command buffers!");
    assert(0 && "failed to allocate command buffers!");
  }

  VkCommandBufferBeginInfo beginInfo = {0};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;               // Optional
  beginInfo.pInheritanceInfo = NULL; // Optional

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    fprintf(stderr, "failed to begin recording command buffer!");
    assert(0 && "failed to begin recording command buffer!");
  }

  // ==================== SYNC OBJECTS ====================
  VkSemaphoreCreateInfo semaphoreInfo = {0};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VkFenceCreateInfo fenceInfo = {0};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  VkSemaphore imageAvailableSemaphore;
  VkSemaphore renderFinishedSemaphore;
  VkFence inFlightFence;
  if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, NULL,
                        &imageAvailableSemaphore) != VK_SUCCESS ||
      vkCreateSemaphore(logicalDevice, &semaphoreInfo, NULL,
                        &renderFinishedSemaphore) != VK_SUCCESS ||
      vkCreateFence(logicalDevice, &fenceInfo, NULL, &inFlightFence) !=
          VK_SUCCESS) {
    fprintf(stderr, "failed to create semaphores!");
  }

  //
  // things that dont go away virtual device
  // swap chain
  // example device
  // https://github.com/KhronosGroup/Vulkan-Samples/blob/main/framework/core/device.h
  // https://github.com/lonelydevil/vulkan-tutorial-C-implementation/blob/main/main.c
  // ==================== MAIN LOOP ====================
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    // ==================== DRAW FRAME ====================
    vkWaitForFences(logicalDevice, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(logicalDevice, 1, &inFlightFence);
    uint32_t imageIndex;
    vkAcquireNextImageKHR(logicalDevice, swapChain, UINT64_MAX,
                          imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    vkResetCommandBuffer(commandBuffer, 0);
    recordCommandBuffer(commandBuffer, imageIndex, renderPass,
                        swapChainFramebuffers, swapChainExtent,
                        graphicsPipeline);

    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) !=
        VK_SUCCESS) {
      fprintf(stderr, "failed to submit draw command buffer!");
    }
    VkPresentInfoKHR presentInfo = {0};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = NULL; // Optional
    vkQueuePresentKHR(presentQueue, &presentInfo);

    vkDeviceWaitIdle(logicalDevice);
  }

  // ==================== CLEAN UP ====================
  if (enableValidationLayers) {
    DestroyDebugUtilsMessengerEXT(vkInstance, debugMessenger, NULL);
  }
  for (size_t i = 0; i < swapChainImageCount; i++) {
    vkDestroyImageView(logicalDevice, swapChainImageViews[i], NULL);
    vkDestroyFramebuffer(logicalDevice, swapChainFramebuffers[i], NULL);
  }
  free(swapChainImageViews);
  free(swapChainFramebuffers);
  vkDestroyPipelineLayout(logicalDevice, pipelineLayout, NULL);
  vkDestroyShaderModule(logicalDevice, fragShaderModule, NULL);
  vkDestroyShaderModule(logicalDevice, vertShaderModule, NULL);
  vkDestroySemaphore(logicalDevice, imageAvailableSemaphore, NULL);
  vkDestroySemaphore(logicalDevice, renderFinishedSemaphore, NULL);
  vkDestroyFence(logicalDevice, inFlightFence, NULL);
  vkDestroyCommandPool(logicalDevice, commandPool, NULL);
  vkDestroyPipeline(logicalDevice, graphicsPipeline, NULL);
  vkDestroyRenderPass(logicalDevice, renderPass, NULL);
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
