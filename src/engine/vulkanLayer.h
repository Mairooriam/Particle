#pragma once
#include "internal/math/raymath.h"
#include "shared.h"
#include <stdalign.h>
#include <stdint.h>
#include <vulkan/vulkan_core.h>
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw3native.h>
// #include "vulkan/vulkan.h"
#include "shared/shared.h"
extern const int enableValidationLayers;

// TODO: add some other define as well?
#if defined(SLOW_CODE_ALLOWED)
extern PFN_vkCreateInstance g_real_vkCreateInstance;
extern VkDebugUtilsMessengerEXT g_debugMessenger;
extern PFN_vkCreateDebugUtilsMessengerEXT g_createDebugUtils;
extern PFN_vkDestroyDebugUtilsMessengerEXT g_destroyDebugUtils;
#endif

typedef struct {
  VkSurfaceCapabilitiesKHR capabilities;
  VkSurfaceFormatKHR *formats;
  uint32_t formatsCount;
  VkPresentModeKHR *presentModes;
  uint32_t presentModesCount;
} SwapChainSupportDetails;

typedef struct {
  uint32_t *code;
  size_t size;
} ShaderCode;

#define MAX_FRAMES_IN_FLIGHT 2 // for command buffer count;
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

// TODO: not good check out storage buffers
#define MAX_INSTANCES 10
typedef struct {
  alignas(16) Matrix models[MAX_INSTANCES];
  alignas(16) Vector4 colors[MAX_INSTANCES];
  alignas(16) Matrix view;
  alignas(16) Matrix proj;
} UniformBufferObject;

typedef struct {
  VkInstance vkInstance;

  GLFWwindow *window;
  VkPhysicalDevice pDevice;
  VkDevice lDevice; // LogicalDevice
  QueueFamilyIndices lDeviceIndices;

  VkSwapchainKHR swapChain;
  VkRenderPass renderPass;
  VkPipeline graphicsPipeline;
  VkFramebuffer *swapChainFramebuffers; // MALLOCED TODO: remove malloc
  VkExtent2D swapChainExtent;
  VkSurfaceKHR surface;

  VkDescriptorSetLayout descriptorSetLayout;
  VkPipelineLayout pipelineLayout;

  VkCommandPool commandPool;
  VkImageView *swapChainImageViews;
  uint32_t swapChainImageCount;

  // Shader
  VkShaderModule vertShaderModule;
  VkShaderModule fragShaderModule;
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  VkBuffer indexBuffer;
  VkDeviceMemory indexBufferMemory;
  Vertex *vertices[2][3];
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  VkDeviceSize stagingBufferSize;

  VkBuffer uniformBuffers[MAX_FRAMES_IN_FLIGHT];
  VkDeviceMemory uniformBuffersMemory[MAX_FRAMES_IN_FLIGHT];
  void *uniformBuffersMapped[MAX_FRAMES_IN_FLIGHT];

  VkDescriptorPool descriptorPool;
  VkDescriptorSet descriptorSets[MAX_FRAMES_IN_FLIGHT];

  // SWAP CHAIN
  VkSurfaceFormatKHR swapChainSurfaceFormat;
  bool framebufferResized;

  // TDOO: move all stuff used in draw frame to drawFrame ctx;
  VkQueue presentQueue;
  VkQueue graphicsQueue;

  // Command buffer
  VkCommandBuffer cmdBuffers[MAX_FRAMES_IN_FLIGHT];
  VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
  VkSemaphore renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
  VkFence inFlightFences[MAX_FRAMES_IN_FLIGHT];
  uint32_t currentFrame;

  // debug
  // VkDebugUtilsMessengerEXT debugMessenger;
  // PFN_vkCreateInstance real_vkCreateInstance;
  // PFN_vkCreateDebugUtilsMessengerEXT createDebugUtils;
  // PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugUtils;
  //
} vulkanContext;
void vkInit(vulkanContext *ctx, GLFWwindow *_window,
            const Vertex *initialVertices, size_t vertexCount,
            const uint16_t *initialIndices, size_t indexCount);

// ==================== DEBUG STUFF ====================
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);
VkResult CreateDebugUtilsMessengerEXT(
    vulkanContext *ctx, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(vulkanContext *ctx,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator);
void populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT *createInfo);

// ==================== HELPERS ====================
uint32_t findMemoryType(VkPhysicalDevice pDevice, uint32_t typeFilter,
                        VkMemoryPropertyFlags properties);
bool checkValidationLayerSupport(const char **requiredLayers,
                                 uint32_t requiredCount);
int getRequiredExtensions(arr_cstr *extensions, bool enableValidationLayers);
void setupDebugMessenger(vulkanContext *ctx, bool enableValidationLayers);
int rateDeviceSuitability(VkPhysicalDevice device);
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device,
                                     VkSurfaceKHR surface);
bool checkDeviceExtensionSupport(VkPhysicalDevice device,
                                 const char **requiredExtensions,
                                 uint32_t requiredCount);
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device,
                                              VkSurfaceKHR surface);
bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface,
                      const char **requiredExtensions,
                      size_t requiredExtensionsCount);
size_t getFileSize(const char *filename);
int readShaderFile(const char *filename, ShaderCode *shader);
VkCommandBuffer beginSingleTimeCommands(vulkanContext *ctx);
void endSingleTimeCommands(vulkanContext *ctx, VkCommandBuffer commandBuffer);

// ==================== NOT SO HELPERS ====================
VkShaderModule createShaderModule(const ShaderCode *shader, VkDevice device);
void updateVertexBuffer(vulkanContext *ctx, const Vertex *newVertices,
                        size_t vertexCount);
void createIndexBuffer(vulkanContext *ctx, uint16_t indicies[], size_t count);

void recordCommandBuffer(vulkanContext *ctx, VkCommandBuffer cmdBuffer,
                         uint32_t imageIndex, size_t indiceCount,
                         arr_Matrix *transforms);
void createSwapChain_and_imageviews(vulkanContext *ctx);
void createFrameBuffers(vulkanContext *ctx);
void cleanupSwapChain(vulkanContext *ctx);
void recreateSwapChain(vulkanContext *ctx);
void vkDrawFrame(vulkanContext *ctx, const Vertex *vertices,
                 uint32_t vertexCount, size_t indiceCount,
                 arr_Matrix *transforms, Vector4 *colors);
void vkCleanup(vulkanContext *ctx);
VkResult vkCreateInstance(const VkInstanceCreateInfo *pCreateInfo,
                          const VkAllocationCallbacks *pAllocator,
                          VkInstance *pInstance);
void createBuffer(vulkanContext *ctx, VkDeviceSize size,
                  VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                  VkBuffer *buffer, VkDeviceMemory *bufferMemory);
void copyBuffer(vulkanContext *ctx, VkBuffer srcBuffer, VkBuffer dstBuffer,
                VkDeviceSize size);

void createDescriptorSetLayout(vulkanContext *ctx);
void createGraphicsPipeline(vulkanContext *ctx);
void createUniformBuffers(vulkanContext *ctx);
void updateUniformBuffer(vulkanContext *ctx, uint32_t currentImage,
                         arr_Matrix *transforms, Vector4 *colors);
void createDescriptorPool(vulkanContext *ctx);
void createDescriptorSets(vulkanContext *ctx);
