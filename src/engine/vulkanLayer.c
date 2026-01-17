#include <corecrt_math_defines.h>
#include "vulkanLayer.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>
#include "cglm/affine2d.h"
#include "cglm/cam.h"
#include "cglm/mat4.h"
#include "utils.h"
#include "shared.h"
#include "time.h"
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw3native.h>
#include "vulkan/vulkan.h"
#ifdef SLOW_CODE_ALLOWED
const int enableValidationLayers = 1;
#else
const int enableValidationLayers = 0;
#endif

#if defined(VK_EXT_debug_utils) && defined(SLOW_CODE_ALLOWED)
PFN_vkCreateInstance g_real_vkCreateInstance = NULL;
VkDebugUtilsMessengerEXT g_debugMessenger = VK_NULL_HANDLE;
PFN_vkCreateDebugUtilsMessengerEXT g_createDebugUtils = NULL;
PFN_vkDestroyDebugUtilsMessengerEXT g_destroyDebugUtils = NULL;
#endif

uint32_t findMemoryType(VkPhysicalDevice pDevice, uint32_t typeFilter,
                        VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(pDevice, &memProperties);
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
      return i;
    }
  }

  fprintf(stderr, "failed to find suitable memory type!\n");
  assert(0 && "failed to find suitable memory type!\n");
  return UINT32_MAX;
}

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
    vulkanContext *ctx, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger) {
  g_createDebugUtils =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          ctx->vkInstance, "vkCreateDebugUtilsMessengerEXT");
  if (g_createDebugUtils != NULL) {
    return g_createDebugUtils(ctx->vkInstance, pCreateInfo, pAllocator,
                              pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugUtilsMessengerEXT(vulkanContext *ctx,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator) {
  g_destroyDebugUtils =
      (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          ctx->vkInstance, "vkDestroyDebugUtilsMessengerEXT");
  if (g_destroyDebugUtils != NULL) {
    g_destroyDebugUtils(ctx->vkInstance, debugMessenger, pAllocator);
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

void setupDebugMessenger(vulkanContext *ctx, bool enableValidationLayers) {
  if (!enableValidationLayers)
    return;

  VkDebugUtilsMessengerCreateInfoEXT createInfo = {0};
  populateDebugMessengerCreateInfo(&createInfo);

  if (CreateDebugUtilsMessengerEXT(ctx, &createInfo, NULL, &debugMessenger) !=
      VK_SUCCESS) {
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

void updateVertexBuffer(vulkanContext *ctx, const Vertex *newVertices,
                        size_t vertexCount) {
  VkDeviceSize bufferSize = sizeof(Vertex) * vertexCount;
  if (bufferSize > ctx->stagingBufferSize) {
    fprintf(stderr, "Staging buffer too small! Need: %zu, Have: %zu\n",
            (size_t)bufferSize, (size_t)ctx->stagingBufferSize);
    return; // Resize or error
  }

  // Map staging buffer and copy data
  void *data;
  vkMapMemory(ctx->lDevice, ctx->stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, newVertices, (size_t)bufferSize);
  vkUnmapMemory(ctx->lDevice, ctx->stagingBufferMemory);

  // Copy from staging to vertex buffer (need command buffer)
  // Assuming you have a transfer queue/command pool
  VkCommandBuffer commandBuffer =
      beginSingleTimeCommands(ctx); // Implement this (one-time command)

  VkBufferCopy copyRegion = {0};
  copyRegion.size = bufferSize;
  vkCmdCopyBuffer(commandBuffer, ctx->stagingBuffer, ctx->vertexBuffer, 1,
                  &copyRegion);

  endSingleTimeCommands(ctx, commandBuffer); // Submit and wait
}

// dont add everything to ctx just the stuff thats used also elsewhere
void recordCommandBuffer(vulkanContext *ctx, VkCommandBuffer cmdBuffer,
                         uint32_t imageIndex, size_t indiceCount) {
  assert(ctx->swapChainFramebuffers &&
         "Dont call record command buffer without frambuffer");
  VkCommandBufferBeginInfo beginInfo = {0};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;               // Optional
  beginInfo.pInheritanceInfo = NULL; // Optional

  if (vkBeginCommandBuffer(cmdBuffer, &beginInfo) != VK_SUCCESS) {
    fprintf(stderr, "failed to begin recording command buffer!");
    assert(0 && "failed to begin recording command buffer!");
  }

  VkRenderPassBeginInfo renderPassInfo = {0};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = ctx->renderPass;
  renderPassInfo.framebuffer = ctx->swapChainFramebuffers[imageIndex];
  renderPassInfo.renderArea.offset = (VkOffset2D){0, 0};
  renderPassInfo.renderArea.extent = ctx->swapChainExtent;

  VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;

  vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    ctx->graphicsPipeline);

  VkBuffer vertexBuffers[] = {ctx->vertexBuffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);
  VkViewport viewport = {0};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = ctx->swapChainExtent.width;
  viewport.height = ctx->swapChainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

  VkRect2D scissor = {0};
  scissor.offset = (VkOffset2D){0, 0};
  scissor.extent = ctx->swapChainExtent;
  vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

  // normal draw and indexed draw
  // vkCmdDraw(cmdBuffer, vertexCount, 1, 0, 0);

  // shader layout stuff
  vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          ctx->pipelineLayout, 0, 1,
                          &ctx->descriptorSets[ctx->currentFrame], 0, NULL);
  // index buffer
  vkCmdBindIndexBuffer(cmdBuffer, ctx->indexBuffer, 0, VK_INDEX_TYPE_UINT16);
  // actual draw
  vkCmdDrawIndexed(cmdBuffer, indiceCount, 1, 0, 0, 0);

  vkCmdEndRenderPass(cmdBuffer);
  if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS) {
    fprintf(stderr, "failed to reconrd command buffer!");
    assert(0 && "failed to create command buffer!");
  }
}

void createSwapChain_and_imageviews(vulkanContext *ctx) {
  // ==================== SWAP CHAIN ====================
  SwapChainSupportDetails swapChainSupport =
      querySwapChainSupport(ctx->pDevice, ctx->surface);

  // ==================== CHOOSE SWAP CHAIN SURFACE FORMAT ====================
  for (size_t i = 0; i < swapChainSupport.formatsCount; i++) {
    if (swapChainSupport.formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
        swapChainSupport.formats[i].colorSpace ==
            VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      ctx->swapChainSurfaceFormat = swapChainSupport.formats[i];
    }
  }
  ctx->swapChainSurfaceFormat = swapChainSupport.formats[0];

  VkPresentModeKHR presentMode;
  for (size_t i = 0; i < swapChainSupport.presentModesCount; i++) {
    if (swapChainSupport.presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      presentMode = swapChainSupport.presentModes[i];
    }
  }
  presentMode = VK_PRESENT_MODE_FIFO_KHR;

  // ==================== CHOOSE SWAP CHAIN EXTENT ====================
  if (swapChainSupport.capabilities.currentExtent.width != UINT32_MAX) {
    ctx->swapChainExtent = swapChainSupport.capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(ctx->window, &width, &height);

    VkExtent2D actualExtent = {(uint32_t)(width), (uint32_t)(height)};

    actualExtent.width = CLAMP(
        actualExtent.width, swapChainSupport.capabilities.minImageExtent.width,
        swapChainSupport.capabilities.maxImageExtent.width);
    actualExtent.height =
        CLAMP(actualExtent.height,
              swapChainSupport.capabilities.minImageExtent.height,
              swapChainSupport.capabilities.maxImageExtent.height);

    ctx->swapChainExtent = actualExtent;
  }

  ctx->swapChainImageCount = swapChainSupport.capabilities.minImageCount + 1;

  if (swapChainSupport.capabilities.maxImageCount > 0 &&
      ctx->swapChainImageCount > swapChainSupport.capabilities.maxImageCount) {
    ctx->swapChainImageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR swapChainCreateInfo = {0};
  swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapChainCreateInfo.surface = ctx->surface;
  swapChainCreateInfo.minImageCount = ctx->swapChainImageCount;
  swapChainCreateInfo.imageFormat = ctx->swapChainSurfaceFormat.format;
  swapChainCreateInfo.imageColorSpace = ctx->swapChainSurfaceFormat.colorSpace;
  swapChainCreateInfo.imageExtent = ctx->swapChainExtent;
  swapChainCreateInfo.imageArrayLayers = 1;
  swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  uint32_t queueFamilyIndices[] = {ctx->lDeviceIndices.graphicsFamily.val,
                                   ctx->lDeviceIndices.presentFamily.val};
  if (ctx->lDeviceIndices.graphicsFamily.val !=
      ctx->lDeviceIndices.presentFamily.val) {
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

  if (vkCreateSwapchainKHR(ctx->lDevice, &swapChainCreateInfo, NULL,
                           &ctx->swapChain) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create swap chain!\n");
    assert(0 && "Failed to create swap chain!");
  }

  vkGetSwapchainImagesKHR(ctx->lDevice, ctx->swapChain,
                          &ctx->swapChainImageCount, NULL);
  VkImage *swapChainImages = malloc(sizeof(VkImage) * ctx->swapChainImageCount);
  if (!swapChainImages) {
    assert(0 && "Lol malloc failed gl");
  }
  vkGetSwapchainImagesKHR(ctx->lDevice, ctx->swapChain,
                          &ctx->swapChainImageCount, swapChainImages);
  // ==================== IMAGE VIEWS ====================
  ctx->swapChainImageViews =
      malloc(sizeof(VkImageView) * ctx->swapChainImageCount);

  for (size_t i = 0; i < ctx->swapChainImageCount; i++) {
    VkImageViewCreateInfo ImageViewCreateInfo = {0};
    ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImageViewCreateInfo.image = swapChainImages[i];
    ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ImageViewCreateInfo.format = ctx->swapChainSurfaceFormat.format;
    ImageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    ImageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    ImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    ImageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    ImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    ImageViewCreateInfo.subresourceRange.levelCount = 1;
    ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    ImageViewCreateInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(ctx->lDevice, &ImageViewCreateInfo, NULL,
                          &ctx->swapChainImageViews[i]) != VK_SUCCESS) {
      fprintf(stderr, "failed to create image views!");
      assert(0 && "failed to create image views!");
    }
  }

  free(swapChainImages);
}

void createFrameBuffers(vulkanContext *ctx) {
  // ==================== FRAMEBUFFERS ====================

  ctx->swapChainFramebuffers =
      malloc(sizeof(VkFramebuffer) * ctx->swapChainImageCount);
  for (size_t i = 0; i < ctx->swapChainImageCount; i++) {
    VkImageView attachments[] = {ctx->swapChainImageViews[i]};

    VkFramebufferCreateInfo framebufferInfo = {0};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = ctx->renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = ctx->swapChainExtent.width;
    framebufferInfo.height = ctx->swapChainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(ctx->lDevice, &framebufferInfo, NULL,
                            &ctx->swapChainFramebuffers[i]) != VK_SUCCESS) {
      fprintf(stderr, "failed to create framebuffer");
      assert(0 && "failed to create framebuffer!");
    }
  }
}
void cleanupSwapChain(vulkanContext *ctx) {
  for (size_t i = 0; i < ctx->swapChainImageCount; i++) {
    vkDestroyFramebuffer(ctx->lDevice, ctx->swapChainFramebuffers[i], NULL);
  }

  for (size_t i = 0; i < ctx->swapChainImageCount; i++) {
    vkDestroyImageView(ctx->lDevice, ctx->swapChainImageViews[i], NULL);
  }

  vkDestroySwapchainKHR(ctx->lDevice, ctx->swapChain, NULL);

  free(ctx->swapChainImageViews);
  free(ctx->swapChainFramebuffers);
}

void recreateSwapChain(vulkanContext *ctx) {
  int width = 0, height = 0;
  glfwGetFramebufferSize(ctx->window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(ctx->window, &width, &height);
    glfwWaitEvents();
  }
  vkDeviceWaitIdle(ctx->lDevice);

  cleanupSwapChain(ctx);

  createSwapChain_and_imageviews(ctx);
  createFrameBuffers(ctx);
}

void vkInit(vulkanContext *ctx, GLFWwindow *_window,
            const Vertex *initialVertices, size_t vertexCount,
            const uint16_t *initialIndices, size_t indexCount) {
  ctx->window = _window;
  ctx->framebufferResized = false;
  glfwSetWindowUserPointer(_window, ctx);

  // ==================== INIT VULKAN ====================
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
  VkApplicationInfo appInfo = {0};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Hello Triangle";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

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

  if (vkCreateInstance(&createInfo, NULL, &ctx->vkInstance) != VK_SUCCESS) {
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
  if (CreateDebugUtilsMessengerEXT(ctx, &createInfoDbg, NULL,
                                   &debugMessenger) != VK_SUCCESS) {
    fprintf(stderr, "Failed to set up debug messanger!");
    // assert(0 && "Failed to setup debug messanger!");
  }
  // ==================== SETUP SURFACE ====================
  VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {0};
  surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  surfaceCreateInfo.hwnd = glfwGetWin32Window(ctx->window);
  surfaceCreateInfo.hinstance = GetModuleHandle(NULL);

  if (vkCreateWin32SurfaceKHR(ctx->vkInstance, &surfaceCreateInfo, NULL,
                              &ctx->surface) != VK_SUCCESS) {
    fprintf(stderr, "failed to create window surface!");
  }
  // ==================== PICK PHYSICAL DEVICE ====================
  ctx->pDevice = VK_NULL_HANDLE;
  uint32_t deviceCount = 0;
  VkResult enumResult =
      vkEnumeratePhysicalDevices(ctx->vkInstance, &deviceCount, NULL);
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

  enumResult =
      vkEnumeratePhysicalDevices(ctx->vkInstance, &deviceCount, devices);
  if (enumResult != VK_SUCCESS) {
    fprintf(stderr, "Failed to enumerate physical devices! Result: %d\n",
            enumResult);
    free(devices);
    assert(0 && "Failed to enumerate physical devices!");
  }
  int bestScore = -1;
  for (size_t i = 0; i < deviceCount; i++) {
    if (!isDeviceSuitable(devices[i], ctx->surface, requiredExtensions,
                          requiredExtensionsCount)) {
      continue;
    }

    int score = rateDeviceSuitability(devices[i]);
    if (score > bestScore) {
      bestScore = score;
      ctx->pDevice = devices[i];
    }
  }

  if (bestScore <= 0 || ctx->pDevice == VK_NULL_HANDLE) {
    fprintf(stderr, "Faiiled to find suitable GPU!\n");
    assert(0 && "failed to find suitable GPU!\n");
  }

  free(devices);

  // ==================== LOGICAL DEVICE ====================

  ctx->lDeviceIndices = findQueueFamilies(ctx->pDevice, ctx->surface);

  float queuePriority = 1.0f;
  VkDeviceQueueCreateInfo
      queueCreateInfos[ctx->lDeviceIndices.queueUniqueFamiliesCount];
  for (size_t i = 0; i < ctx->lDeviceIndices.queueUniqueFamiliesCount; i++) {
    VkDeviceQueueCreateInfo queueCreateInfo = {0};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex =
        ctx->lDeviceIndices.queueUniqueFamilies[i];
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
      ctx->lDeviceIndices.queueUniqueFamiliesCount;
  LogigalCreateInfo.pEnabledFeatures = &deviceFeatures;
  LogigalCreateInfo.enabledExtensionCount = requiredExtensionsCount;
  LogigalCreateInfo.ppEnabledExtensionNames = requiredExtensions;

  if (enableValidationLayers) {
    LogigalCreateInfo.enabledLayerCount = validationLayerCount;
    LogigalCreateInfo.ppEnabledLayerNames = validationLayers;
  } else {
    LogigalCreateInfo.enabledLayerCount = 0;
  }

  if (vkCreateDevice(ctx->pDevice, &LogigalCreateInfo, NULL, &ctx->lDevice) !=
      VK_SUCCESS) {
    fprintf(stderr, "Failed to create logical device!");
    assert(0 && "Failed to create logical device");
  }

  vkGetDeviceQueue(ctx->lDevice, ctx->lDeviceIndices.presentFamily.val, 0,
                   &ctx->presentQueue);
  vkGetDeviceQueue(ctx->lDevice, ctx->lDeviceIndices.graphicsFamily.val, 0,
                   &ctx->graphicsQueue);

  createSwapChain_and_imageviews(ctx);

  createDescriptorSetLayout(ctx);
  createDescriptorPool(ctx);
  createUniformBuffers(ctx);
  createDescriptorSets(ctx);

  // ==================== GRAPGICS PIPELINE - SHADERS ====================
  size_t shadersize = getFileSize("shader/vert2.spv");
  assert(shadersize > 0 && "Is the path valid?");
  ShaderCode vertShaderCode = {0};
  vertShaderCode.code = malloc(shadersize);
  vertShaderCode.size = shadersize;
  shadersize = 0;

  readShaderFile("shader/vert2.spv", &vertShaderCode);

  shadersize = getFileSize("shader/frag2.spv");
  assert(shadersize > 0 && "Is the path valid?");
  ShaderCode fragShaderCode = {0};
  fragShaderCode.code = malloc(shadersize);
  fragShaderCode.size = shadersize;

  readShaderFile("shader/frag2.spv", &fragShaderCode);

  ctx->vertShaderModule = createShaderModule(&vertShaderCode, ctx->lDevice);
  ctx->fragShaderModule = createShaderModule(&fragShaderCode, ctx->lDevice);

  free(vertShaderCode.code);
  free(fragShaderCode.code);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {0};
  vertShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = ctx->vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {0};
  fragShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = ctx->fragShaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                    fragShaderStageInfo};
  // ==================== COMMAND POOLS ====================
  VkCommandPoolCreateInfo poolInfo = {0};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = ctx->lDeviceIndices.graphicsFamily.val;

  if (vkCreateCommandPool(ctx->lDevice, &poolInfo, NULL, &ctx->commandPool) !=
      VK_SUCCESS) {
    fprintf(stderr, "failed to create command pool!");
    assert(0 && "failed to create command pool!");
  }

  VkCommandBufferAllocateInfo cmdBufferAllocInfo = {0};
  cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmdBufferAllocInfo.commandPool = ctx->commandPool;
  cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmdBufferAllocInfo.commandBufferCount = (uint32_t)MAX_FRAMES_IN_FLIGHT;

  if (vkAllocateCommandBuffers(ctx->lDevice, &cmdBufferAllocInfo,
                               ctx->cmdBuffers) != VK_SUCCESS) {
    fprintf(stderr, "failed to allocate command buffers!");
    assert(0 && "failed to allocate command buffers!");
  }

  VkCommandBufferBeginInfo beginInfo = {0};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;               // Optional
  beginInfo.pInheritanceInfo = NULL; // Optional

  if (vkBeginCommandBuffer(ctx->cmdBuffers[ctx->currentFrame], &beginInfo) !=
      VK_SUCCESS) {
    fprintf(stderr, "failed to begin recording command buffer!");
    assert(0 && "failed to begin recording command buffer!");
  }
  // ==================== VERTEX LAYOUT ====================
  VkVertexInputBindingDescription bindingDescription = {0};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(Vertex);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  uint32_t attributeCount = 2;
  VkVertexInputAttributeDescription attributeDescriptions[attributeCount];

  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0; // Location for pos (vec2)
  attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; // 2 floats
  attributeDescriptions[0].offset = offsetof(Vertex, pos);

  attributeDescriptions[1].binding = 0;
  attributeDescriptions[1].location = 1; // Location for color (vec3)
  attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // 3 floats
  attributeDescriptions[1].offset = offsetof(Vertex, color);

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount = attributeCount;
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

  VkDeviceSize bufferSize = sizeof(initialVertices[0]) * vertexCount;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer(ctx, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               &stagingBuffer, &stagingBufferMemory);

  void *data;
  vkMapMemory(ctx->lDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, initialVertices, (size_t)bufferSize);
  vkUnmapMemory(ctx->lDevice, stagingBufferMemory);

  ctx->stagingBuffer = stagingBuffer;
  ctx->stagingBufferMemory = stagingBufferMemory;
  ctx->stagingBufferSize = bufferSize;

  createBuffer(ctx, bufferSize,
               VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &ctx->vertexBuffer,
               &ctx->vertexBufferMemory);

  // TODO: vertexbuffer size into ctx?
  copyBuffer(ctx, stagingBuffer, ctx->vertexBuffer, bufferSize);
  createIndexBuffer(ctx, initialIndices, indexCount);
  // ==================== DYNAMIC STATE ====================
  VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                    VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamicState = {0};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = ARR_COUNT(dynamicStates);
  dynamicState.pDynamicStates = dynamicStates;

  // ==================== INPUT ASSEMBLER ====================
  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

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
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;                      // Optional
  pipelineLayoutInfo.pSetLayouts = &ctx->descriptorSetLayout; // Optional
  pipelineLayoutInfo.pushConstantRangeCount = 0;              // Optional
  pipelineLayoutInfo.pPushConstantRanges = NULL;              // Optional

  if (vkCreatePipelineLayout(ctx->lDevice, &pipelineLayoutInfo, NULL,
                             &ctx->pipelineLayout) != VK_SUCCESS) {
    fprintf(stderr, "failed to create pipeline layout!");
    assert(0 && "failed to create pipeline layout!");
  }

  // ==================== RENDER PASSES ====================
  VkAttachmentDescription colorAttachment = {0};
  colorAttachment.format = ctx->swapChainSurfaceFormat.format;
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

  VkRenderPassCreateInfo renderPassInfo = {0};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (vkCreateRenderPass(ctx->lDevice, &renderPassInfo, NULL,
                         &ctx->renderPass) != VK_SUCCESS) {
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
  pipelineInfo.layout = ctx->pipelineLayout;
  pipelineInfo.renderPass = ctx->renderPass;
  pipelineInfo.subpass =
      0; // https://docs.vulkan.org/spec/latest/chapters/renderpass.html#renderpass-compatibility
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
  pipelineInfo.basePipelineIndex = -1;              // Optional

  if (vkCreateGraphicsPipelines(ctx->lDevice, VK_NULL_HANDLE, 1, &pipelineInfo,
                                NULL, &ctx->graphicsPipeline) != VK_SUCCESS) {
    fprintf(stderr, "failed to create graphics pipeline!");
    assert(0 && "failed to create graphics pipeline!");
  }

  createFrameBuffers(ctx);

  // ==================== SYNC OBJECTS ====================
  VkSemaphoreCreateInfo semaphoreInfo = {0};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo = {0};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(ctx->lDevice, &semaphoreInfo, NULL,
                          &ctx->imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(ctx->lDevice, &semaphoreInfo, NULL,
                          &ctx->renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(ctx->lDevice, &fenceInfo, NULL,
                      &ctx->inFlightFences[i]) != VK_SUCCESS) {
      fprintf(stderr, "failed to create semaphores!");
    }
  }
}

void vkDrawFrame(vulkanContext *ctx, const Vertex *vertices,
                 uint32_t vertexCount, size_t indiceCount) {
  // updateVertexBuffer(ctx, vertices, vertexCount);

  // ==================== DRAW FRAME ====================
  vkWaitForFences(ctx->lDevice, 1, &ctx->inFlightFences[ctx->currentFrame],
                  VK_TRUE, UINT64_MAX);
  uint32_t imageIndex;
  VkResult result =
      vkAcquireNextImageKHR(ctx->lDevice, ctx->swapChain, UINT64_MAX,
                            ctx->imageAvailableSemaphores[ctx->currentFrame],
                            VK_NULL_HANDLE, &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain(ctx);
    return;
  } else if (result != VK_SUCCESS) {
    fprintf(stderr, "failed to acquire swapchain image!\n");
    assert(0 && "failed to acquire swapchain image!");
  }
  vkResetFences(ctx->lDevice, 1, &ctx->inFlightFences[ctx->currentFrame]);

  vkResetCommandBuffer(ctx->cmdBuffers[ctx->currentFrame], 0);
  recordCommandBuffer(ctx, ctx->cmdBuffers[ctx->currentFrame], imageIndex,
                      indiceCount);

  updateUniformBuffer(ctx, ctx->currentFrame);

  VkSubmitInfo submitInfo = {0};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {
      ctx->imageAvailableSemaphores[ctx->currentFrame]};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &ctx->cmdBuffers[ctx->currentFrame];
  VkSemaphore signalSemaphores[] = {
      ctx->renderFinishedSemaphores[ctx->currentFrame]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  if (vkQueueSubmit(ctx->graphicsQueue, 1, &submitInfo,
                    ctx->inFlightFences[ctx->currentFrame]) != VK_SUCCESS) {
    fprintf(stderr, "failed to submit draw command buffer!");
  }
  VkPresentInfoKHR presentInfo = {0};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;
  VkSwapchainKHR swapChains[] = {ctx->swapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;
  presentInfo.pResults = NULL; // Optional
  VkResult presentResult = vkQueuePresentKHR(ctx->presentQueue, &presentInfo);

  if (presentResult == VK_ERROR_OUT_OF_DATE_KHR ||
      presentResult == VK_SUBOPTIMAL_KHR || ctx->framebufferResized) {
    ctx->framebufferResized = false;
    recreateSwapChain(ctx);
    return;
  } else if (presentResult != VK_SUCCESS) {
    fprintf(stderr, "failed to acquire swapchain image!\n");
    assert(0 && "failed to acquire swapchain image!");
  }

  vkDeviceWaitIdle(ctx->lDevice);

  ctx->currentFrame = (ctx->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void vkCleanup(vulkanContext *ctx) {
  // ==================== CLEAN UP ====================
  cleanupSwapChain(ctx);

  vkDestroyBuffer(ctx->lDevice, ctx->vertexBuffer, NULL);
  vkFreeMemory(ctx->lDevice, ctx->vertexBufferMemory, NULL);
  vkDestroyBuffer(ctx->lDevice, ctx->indexBuffer, NULL);
  vkFreeMemory(ctx->lDevice, ctx->indexBufferMemory, NULL);
  vkDestroyDescriptorPool(ctx->lDevice, ctx->descriptorPool, NULL);

  vkDestroyPipeline(ctx->lDevice, ctx->graphicsPipeline, NULL);
  vkDestroyPipelineLayout(ctx->lDevice, ctx->pipelineLayout, NULL);
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroyBuffer(ctx->lDevice, ctx->uniformBuffers[i], NULL);
    vkFreeMemory(ctx->lDevice, ctx->uniformBuffersMemory[i], NULL);
  }

  vkDestroyDescriptorSetLayout(ctx->lDevice, ctx->descriptorSetLayout, NULL);
  vkDestroyRenderPass(ctx->lDevice, ctx->renderPass, NULL);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(ctx->lDevice, ctx->imageAvailableSemaphores[i], NULL);
    vkDestroySemaphore(ctx->lDevice, ctx->renderFinishedSemaphores[i], NULL);
    vkDestroyFence(ctx->lDevice, ctx->inFlightFences[i], NULL);
  }

  // SHOULD THESWE MVOE?
  vkDestroyShaderModule(ctx->lDevice, ctx->fragShaderModule, NULL);
  vkDestroyShaderModule(ctx->lDevice, ctx->vertShaderModule, NULL);

  vkDestroyCommandPool(ctx->lDevice, ctx->commandPool, NULL);
  vkDestroyBuffer(ctx->lDevice, ctx->stagingBuffer, NULL);
  vkFreeMemory(ctx->lDevice, ctx->stagingBufferMemory, NULL);
  vkDestroyDevice(ctx->lDevice, NULL);

  if (enableValidationLayers) {
    DestroyDebugUtilsMessengerEXT(ctx, debugMessenger, NULL);
  }

  vkDestroySurfaceKHR(ctx->vkInstance, ctx->surface, NULL);
  vkDestroyInstance(ctx->vkInstance, NULL);
}
VkResult vkCreateInstance(const VkInstanceCreateInfo *pCreateInfo,
                          const VkAllocationCallbacks *pAllocator,
                          VkInstance *pInstance) {

  printf("hello from debug vkCreateInstance\n");
  if (pCreateInfo == NULL || pInstance == NULL) {
    fprintf(stderr, "Null pointer passed to required parameter!\n");
    return VK_ERROR_INITIALIZATION_FAILED;
  }
#if defined(SLOW_CODE_ALLOWED)
  if (g_real_vkCreateInstance) {
    return g_real_vkCreateInstance(pCreateInfo, pAllocator, pInstance);
  }
#endif
  return VK_ERROR_INITIALIZATION_FAILED;
}

void createBuffer(vulkanContext *ctx, VkDeviceSize size,
                  VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                  VkBuffer *buffer, VkDeviceMemory *bufferMemory) {
  VkBufferCreateInfo bufferInfo = {0};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(ctx->lDevice, &bufferInfo, NULL, buffer) != VK_SUCCESS) {
    fprintf(stderr, "failed to create vertex buffer!\n");
    assert(0 && "failed to create vertex buffer!");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(ctx->lDevice, *buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      findMemoryType(ctx->pDevice, memRequirements.memoryTypeBits, properties);
  if (vkAllocateMemory(ctx->lDevice, &allocInfo, NULL, bufferMemory) !=
      VK_SUCCESS) {
    fprintf(stderr, "failed to allocate vertex buffer memory!\n");
    assert(0 && "failed to allocate vertex buffer memory!\n");
  }
  vkBindBufferMemory(ctx->lDevice, *buffer, *bufferMemory, 0);
}

void copyBuffer(vulkanContext *ctx, VkBuffer srcBuffer, VkBuffer dstBuffer,
                VkDeviceSize size) {

  VkCommandBuffer commandBuffer = beginSingleTimeCommands(ctx);

  VkBufferCopy copyRegion = {0};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  endSingleTimeCommands(ctx, commandBuffer);
}

VkCommandBuffer beginSingleTimeCommands(vulkanContext *ctx) {
  VkCommandBufferAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = ctx->commandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer = {0};
  vkAllocateCommandBuffers(ctx->lDevice, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo = {0};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);
  return commandBuffer;
}
void endSingleTimeCommands(vulkanContext *ctx, VkCommandBuffer commandBuffer) {
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo = {0};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  // TODO: look into transfer ques
  vkQueueSubmit(ctx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(ctx->graphicsQueue);

  vkFreeCommandBuffers(ctx->lDevice, ctx->commandPool, 1, &commandBuffer);
}

void createIndexBuffer(vulkanContext *ctx, uint16_t indices[], size_t count) {
  VkDeviceSize bufferSize = sizeof(uint16_t) * count;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer(ctx, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               &stagingBuffer, &stagingBufferMemory);

  void *data;
  vkMapMemory(ctx->lDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, indices, (size_t)bufferSize);
  vkUnmapMemory(ctx->lDevice, stagingBufferMemory);

  createBuffer(ctx, bufferSize,
               VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &ctx->indexBuffer,
               &ctx->indexBufferMemory);

  copyBuffer(ctx, stagingBuffer, ctx->indexBuffer, bufferSize);

  vkDestroyBuffer(ctx->lDevice, stagingBuffer, NULL);
  vkFreeMemory(ctx->lDevice, stagingBufferMemory, NULL);
}

void createDescriptorSetLayout(vulkanContext *ctx) {
  VkDescriptorSetLayoutBinding uboLayoutBinding = {0};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  uboLayoutBinding.pImmutableSamplers = NULL;

  VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &uboLayoutBinding;

  if (vkCreateDescriptorSetLayout(ctx->lDevice, &layoutInfo, NULL,
                                  &ctx->descriptorSetLayout) != VK_SUCCESS) {
    fprintf(stderr, "failed to create descriptor set layout!");
    assert(0 && "Failed to create descriptor set layout!");
  }
}

void createUniformBuffers(vulkanContext *ctx) {

  VkDeviceSize bufferSize = sizeof(UniformBufferObject);
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    createBuffer(ctx, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &ctx->uniformBuffers[i], &ctx->uniformBuffersMemory[i]);
    vkMapMemory(ctx->lDevice, ctx->uniformBuffersMemory[i], 0, bufferSize, 0,
                &ctx->uniformBuffersMapped[i]);
  }
}

// TODO: abstract windows.h away just generic getTime()
void updateUniformBuffer(vulkanContext *ctx, uint32_t currentImage) {
  static LARGE_INTEGER frequency;
  static LARGE_INTEGER startTime;
  static int initialized = 0;

  if (!initialized) {
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&startTime);
    initialized = 1;
  }
  float aspect =
      (float)ctx->swapChainExtent.width / (float)ctx->swapChainExtent.height;
  LARGE_INTEGER currentTime;
  QueryPerformanceCounter(&currentTime);

  float time = (float)(currentTime.QuadPart - startTime.QuadPart) /
               (float)frequency.QuadPart;

  UniformBufferObject ubo = {0};
  // glm_mat4_identity(ubo.model);
  // glm_rotate(ubo.model, glm_rad(90.0f) * time, (vec3){0.0f, 0.0f, 1.0f});

  // glm_lookat((vec3){2.0f, 2.0f, 2.0f}, // Eye position
  //            (vec3){0.0f, 0.0f, 0.0f}, // Target position
  //            (vec3){0.0f, 0.0f, 1.0f}, // Up vector
  //            ubo.view);                // Output view matrix
  // glm_perspective(glm_rad(45.0f), aspect, 0.1f, 10.0f, ubo.proj);
  // ubo.proj[1][1] *= -1;

  glm_mat4_identity(ubo.model);
  mat4 rotation;
  glm_mat4_zero(rotation);
  // rotation[0][0] = 0.0f;
  // rotation[0][1] = 1.0f;
  // rotation[1][0] = -1.0f;
  // rotation[1][1] = 0.0f;
  // rotation[2][2] = 1.0f;
  // rotation[3][3] = 1.0f;

  float rot = M_PI;
  rotation[0][0] = sin(rot);
  rotation[0][1] = cos(rot);
  rotation[1][0] = cos(rot);
  rotation[1][1] = -sin(rot);
  rotation[2][2] = 1.0f;
  rotation[3][3] = 1.0f;
  glm_mat4_mul(rotation, ubo.model, ubo.model);

  mat4 shear;
  glm_mat4_zero(shear);
  shear[0][0] = 1.0f;
  shear[0][1] = 0.0f;
  shear[1][0] = 1.0f;
  shear[1][1] = 1.0f;
  shear[2][2] = 1.0f;
  shear[3][3] = 1.0f;
  glm_mat4_mul(shear, ubo.model, ubo.model);

  glm_mat4_print(ubo.model, stdout);

  glm_mat4_identity(ubo.view);
  glm_lookat((vec3){0.0f, 0.0f, 3.0f}, // Eye: 5 units back
             (vec3){0.0f, 0.0f, 0.0f}, // Target: origin
             (vec3){0.0f, 1.0f, 0.0f}, // Up: Y-up
             ubo.view);

  float size = 1.5f;
  // glm_ortho(-size, size, -size / aspect, size / aspect, -1.0f, 1.0f,
  // ubo.proj);
  glm_perspective(glm_rad(45.0f), aspect, 0.1f, 10.0f, ubo.proj);
  ubo.proj[1][1] *= -1; // Vulkan clip space adjustment
  memcpy(ctx->uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}
void createDescriptorPool(vulkanContext *ctx) {
  VkDescriptorPoolSize poolSize = {0};
  poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

  VkDescriptorPoolCreateInfo poolInfo = {0};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;

  if (vkCreateDescriptorPool(ctx->lDevice, &poolInfo, NULL,
                             &ctx->descriptorPool) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create descriptor pool!\n");
    assert(0 && "Failed to create descriptor pool!");
  }
}

void createDescriptorSets(vulkanContext *ctx) {
  VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT];
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    layouts[i] = ctx->descriptorSetLayout;
  }

  VkDescriptorSetAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = ctx->descriptorPool;
  allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
  allocInfo.pSetLayouts = layouts;

  if (vkAllocateDescriptorSets(ctx->lDevice, &allocInfo, ctx->descriptorSets) !=
      VK_SUCCESS) {
    fprintf(stderr, "Failed to allocate descriptor sets!\n");
    assert(0 && "Failed to allocate descriptor sets!");
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo bufferInfo = {0};
    bufferInfo.buffer = ctx->uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkWriteDescriptorSet descriptorWrite = {0};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = ctx->descriptorSets[i];
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;
    descriptorWrite.pImageInfo = NULL;
    descriptorWrite.pTexelBufferView = NULL;

    vkUpdateDescriptorSets(ctx->lDevice, 1, &descriptorWrite, 0, NULL);
  }
}
