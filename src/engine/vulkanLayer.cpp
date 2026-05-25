/* Copyright (c) 2025-2026, Sascha Willems
 * SPDX-License-Identifier: MIT
 */
#include "vulkanLayer.h"
#include "core/log.h"
#include "internal/renderQue.h"
#include "shared.h"
#include <cstdint>
#include <memory>
#define VOLK_IMPLEMENTATION
#include <SDL3/SDL_vulkan.h>
#include <array>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <volk/volk.h>
#include <vulkan/vulkan.h>
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "slang/slang-com-ptr.h"
#include "slang/slang.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <ktx.h>
#include <ktxvulkan.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

constexpr uint32_t maxFramesInFlight{2};
uint32_t imageIndex{0};
uint32_t frameIndex{0};
VkInstance instance{VK_NULL_HANDLE};
VkDevice device{VK_NULL_HANDLE};
VkQueue queue{VK_NULL_HANDLE};
VkSurfaceKHR surface{VK_NULL_HANDLE};
VkCommandPool commandPool{VK_NULL_HANDLE};
VkPipeline pipeline{VK_NULL_HANDLE};
VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};
VkImage depthImage;
VmaAllocator allocator{VK_NULL_HANDLE};
VmaAllocation depthImageAllocation;
VkImageView depthImageView;
std::vector<VkImage> swapchainImages;
std::vector<VkImageView> swapchainImageViews;
std::array<VkCommandBuffer, maxFramesInFlight> commandBuffers;
std::array<VkFence, maxFramesInFlight> fences;
std::array<VkSemaphore, maxFramesInFlight> presentSemaphores;
std::vector<VkSemaphore> renderSemaphores;
// vBufferAllocation removed: buffer now owned by vulkanContext::mesh.vertices
//
//

struct ShaderDataBuffer {
  VmaAllocation allocation{VK_NULL_HANDLE};
  VmaAllocationInfo allocationInfo{};
  VkBuffer buffer{VK_NULL_HANDLE};
  VkDeviceAddress deviceAddress{};
};
std::array<ShaderDataBuffer, maxFramesInFlight> shaderDataBuffers;
struct Texture {
  VmaAllocation allocation{VK_NULL_HANDLE};
  VkImage image{VK_NULL_HANDLE};
  VkImageView view{VK_NULL_HANDLE};
  VkSampler sampler{VK_NULL_HANDLE};
};
std::array<Texture, 3> textures{};
VkDescriptorPool descriptorPool{VK_NULL_HANDLE};
VkDescriptorSetLayout descriptorSetLayoutTex{VK_NULL_HANDLE};
VkDescriptorSet descriptorSetTex{VK_NULL_HANDLE};
Slang::ComPtr<slang::IGlobalSession> slangGlobalSession;
struct Vertex {
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 uv;
};

static inline void chkImpl(VkResult result, const char *file, int line,
                           const char *expr) {
  if (result != VK_SUCCESS) {
    std::cerr << "Vulkan error (" << result << ")\n  expr: " << expr << "\n  "
              << file << ":" << line << "\n";
    exit(result);
  }
}
static inline void chkSwapchainImpl(VkResult result, bool &updateSwapchain,
                                    const char *file, int line,
                                    const char *expr) {
  if (result < VK_SUCCESS) {
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      updateSwapchain = true;
      return;
    }
    std::cerr << "Vulkan error (" << result << ")\n  expr: " << expr << "\n  "
              << file << ":" << line << "\n";
    exit(result);
  }
}
static inline void chkImpl(bool result, const char *file, int line,
                           const char *expr) {
  if (!result) {
    std::cerr << "Call failed\n  expr: " << expr << "\n  " << file << ":"
              << line << "\n";
    exit(1);
  }
}
#define chk(x) chkImpl((x), __FILE__, __LINE__, #x)
#define chkSwapchain(x, updateSwapchain)                                       \
  chkSwapchainImpl((x), updateSwapchain, __FILE__, __LINE__, #x)
void init(vulkanContext *ctx, size_t shaderDataSize) {
  chk(SDL_Init(SDL_INIT_VIDEO));
  chk(SDL_Vulkan_LoadLibrary(NULL));
  volkInitialize();
  // Instance
  VkApplicationInfo appInfo{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                            .pApplicationName = "How to Vulkan",
                            .apiVersion = VK_API_VERSION_1_3};
  uint32_t instanceExtensionsCount{0};
  char const *const *instanceExtensions{
      SDL_Vulkan_GetInstanceExtensions(&instanceExtensionsCount)};
  VkInstanceCreateInfo instanceCI{
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &appInfo,
      .enabledExtensionCount = instanceExtensionsCount,
      .ppEnabledExtensionNames = instanceExtensions,
  };
  chk(vkCreateInstance(&instanceCI, nullptr, &instance));
  volkLoadInstance(instance);
  // Device
  uint32_t deviceCount{0};
  chk(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));

  ctx->devices.resize(deviceCount);
  chk(vkEnumeratePhysicalDevices(instance, &deviceCount, ctx->devices.data()));
  // TODO: add proper score selection
  //
  ctx->deviceIndex = 0;
  // if (argc > 1) {
  //   deviceIndex = std::stoi(argv[1]);
  //   assert(deviceIndex < deviceCount);
  // }
  VkPhysicalDeviceProperties2 deviceProperties{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
  vkGetPhysicalDeviceProperties2(ctx->devices[ctx->deviceIndex],
                                 &deviceProperties);
  std::cout << "Selected device: " << deviceProperties.properties.deviceName
            << "\n";
  // Find a queue family for graphics
  uint32_t queueFamilyCount{0};
  vkGetPhysicalDeviceQueueFamilyProperties(ctx->devices[ctx->deviceIndex],
                                           &queueFamilyCount, nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(
      ctx->devices[ctx->deviceIndex], &queueFamilyCount, queueFamilies.data());
  uint32_t queueFamily{0};
  for (size_t i = 0; i < queueFamilies.size(); i++) {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      queueFamily = i;
      break;
    }
  }
  std::cout << "Queue family: " << queueFamily << "\n";
  bool presentSupported = SDL_Vulkan_GetPresentationSupport(
      instance, ctx->devices[ctx->deviceIndex], queueFamily);
  std::cout << "Present support: " << presentSupported << "\n";
  chk(presentSupported);
  // Logical device
  const float qfpriorities{1.0f};
  VkDeviceQueueCreateInfo queueCI{
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = queueFamily,
      .queueCount = 1,
      .pQueuePriorities = &qfpriorities};
  VkPhysicalDeviceVulkan12Features enabledVk12Features{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
      .descriptorIndexing = true,
      .shaderSampledImageArrayNonUniformIndexing = true,
      .descriptorBindingVariableDescriptorCount = true,
      .runtimeDescriptorArray = true,
      .bufferDeviceAddress = true};
  VkPhysicalDeviceVulkan13Features enabledVk13Features{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
      .pNext = &enabledVk12Features,
      .synchronization2 = true,
      .dynamicRendering = true};
  const std::vector<const char *> deviceExtensions{
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  const VkPhysicalDeviceFeatures enabledVk10Features{.samplerAnisotropy =
                                                         VK_TRUE};
  VkDeviceCreateInfo deviceCI{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = &enabledVk13Features,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &queueCI,
      .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
      .ppEnabledExtensionNames = deviceExtensions.data(),
      .pEnabledFeatures = &enabledVk10Features};
  chk(vkCreateDevice(ctx->devices[ctx->deviceIndex], &deviceCI, nullptr,
                     &device));
  vkGetDeviceQueue(device, queueFamily, 0, &queue);
  // VMA
  VmaVulkanFunctions vkFunctions{.vkGetInstanceProcAddr = vkGetInstanceProcAddr,
                                 .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
                                 .vkCreateImage = vkCreateImage};
  VmaAllocatorCreateInfo allocatorCI{
      .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
      .physicalDevice = ctx->devices[ctx->deviceIndex],
      .device = device,
      .pVulkanFunctions = &vkFunctions,
      .instance = instance};
  chk(vmaCreateAllocator(&allocatorCI, &allocator));
  // Window and surface
  ctx->window =
      SDL_CreateWindow("How to Vulkan", ctx->windowSize[0], ctx->windowSize[1],
                       SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
  assert(ctx->window);
  chk(SDL_Vulkan_CreateSurface(ctx->window, instance, nullptr, &surface));
  chk(SDL_GetWindowSize(ctx->window, &ctx->windowSize[0], &ctx->windowSize[1]));
  chk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->devices[ctx->deviceIndex],
                                                surface, &ctx->surfaceCaps));
  VkExtent2D swapchainExtent{ctx->surfaceCaps.currentExtent};
  if (ctx->surfaceCaps.currentExtent.width == 0xFFFFFFFF) {
    swapchainExtent = {.width = static_cast<uint32_t>(ctx->windowSize[0]),
                       .height = static_cast<uint32_t>(ctx->windowSize[1])};
  }
  // Swap chain
  ctx->imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
  ctx->swapchainCI = {.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                      .surface = surface,
                      .minImageCount = ctx->surfaceCaps.minImageCount,
                      .imageFormat = ctx->imageFormat,
                      .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
                      .imageExtent{.width = swapchainExtent.width,
                                   .height = swapchainExtent.height},
                      .imageArrayLayers = 1,
                      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                      .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
                      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                      .presentMode = VK_PRESENT_MODE_FIFO_KHR};
  chk(vkCreateSwapchainKHR(device, &ctx->swapchainCI, nullptr,
                           &ctx->swapchain));
  ctx->imageCount = 0;
  chk(vkGetSwapchainImagesKHR(device, ctx->swapchain, &ctx->imageCount,
                              nullptr));
  swapchainImages.resize(ctx->imageCount);
  chk(vkGetSwapchainImagesKHR(device, ctx->swapchain, &ctx->imageCount,
                              swapchainImages.data()));
  swapchainImageViews.resize(ctx->imageCount);
  for (auto i = 0; i < ctx->imageCount; i++) {
    VkImageViewCreateInfo viewCI{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = swapchainImages[i],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = ctx->imageFormat,
        .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                          .levelCount = 1,
                          .layerCount = 1}};
    chk(vkCreateImageView(device, &viewCI, nullptr, &swapchainImageViews[i]));
  }
  // Depth attachment
  std::vector<VkFormat> depthFormatList{VK_FORMAT_D32_SFLOAT_S8_UINT,
                                        VK_FORMAT_D24_UNORM_S8_UINT};
  for (VkFormat &format : depthFormatList) {
    VkFormatProperties2 formatProperties{
        .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2};
    vkGetPhysicalDeviceFormatProperties2(ctx->devices[ctx->deviceIndex], format,
                                         &formatProperties);
    if (formatProperties.formatProperties.optimalTilingFeatures &
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      ctx->depthFormat = format;
      break;
    }
  }
  assert(ctx->depthFormat != VK_FORMAT_UNDEFINED);
  ctx->depthImageCI = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = ctx->depthFormat,
      .extent{.width = static_cast<uint32_t>(ctx->windowSize[0]),
              .height = static_cast<uint32_t>(ctx->windowSize[1]),
              .depth = 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };
  VmaAllocationCreateInfo allocCI{
      .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
      .usage = VMA_MEMORY_USAGE_AUTO};
  chk(vmaCreateImage(allocator, &ctx->depthImageCI, &allocCI, &depthImage,
                     &depthImageAllocation, nullptr));
  VkImageViewCreateInfo depthViewCI{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = depthImage,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = ctx->depthFormat,
      .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                        .levelCount = 1,
                        .layerCount = 1}};
  chk(vkCreateImageView(device, &depthViewCI, nullptr, &depthImageView));
  // Mesh data
  // FIRST MESH
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  chk(tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, nullptr,
                       "assets/suzanne.obj"));
  ctx->indexCount = shapes[0].mesh.indices.size();
  std::vector<Vertex> vertices{};
  std::vector<uint16_t> indices{};
  // Load vertex and index data
  for (auto &index : shapes[0].mesh.indices) {
    Vertex v{.pos = {attrib.vertices[index.vertex_index * 3],
                     -attrib.vertices[index.vertex_index * 3 + 1],
                     attrib.vertices[index.vertex_index * 3 + 2]},
             .normal = {attrib.normals[index.normal_index * 3],
                        -attrib.normals[index.normal_index * 3 + 1],
                        attrib.normals[index.normal_index * 3 + 2]},
             .uv = {attrib.texcoords[index.texcoord_index * 2],
                    1.0 - attrib.texcoords[index.texcoord_index * 2 + 1]}};
    vertices.push_back(v);
    indices.push_back(indices.size());
  }
  // TIME TO MAP TO VULKAN

  ctx->mesh.vertices.size = sizeof(Vertex) * vertices.size();
  ctx->mesh.indicesSize = sizeof(uint16_t) * indices.size();
  ctx->mesh.indexCount = static_cast<uint32_t>(indices.size());

  // Single buffer: [vertex data | index data]
  VkBufferCreateInfo bufferCI{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                              .size = ctx->mesh.vertices.size +
                                      ctx->mesh.indicesSize,
                              .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                       VK_BUFFER_USAGE_INDEX_BUFFER_BIT};
  VmaAllocationCreateInfo vBufferAllocCI{
      .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
               VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
               VMA_ALLOCATION_CREATE_MAPPED_BIT,
      .usage = VMA_MEMORY_USAGE_AUTO};
  chk(vmaCreateBuffer(allocator, &bufferCI, &vBufferAllocCI,
                      &ctx->mesh.vertices.buf, &ctx->mesh.vertices.allocation,
                      &ctx->mesh.vertices.allocationInfo));
  void *mapped = ctx->mesh.vertices.allocationInfo.pMappedData;
  memcpy(mapped, vertices.data(), ctx->mesh.vertices.size);
  memcpy((char *)mapped + ctx->mesh.vertices.size, indices.data(),
         ctx->mesh.indicesSize);
  // Shader data buffers
  for (auto i = 0; i < maxFramesInFlight; i++) {
    VkBufferCreateInfo uBufferCI{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                 .size = shaderDataSize,
                                 .usage =
                                     VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT};
    VmaAllocationCreateInfo uBufferAllocCI{
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                 VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
                 VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO};
    chk(vmaCreateBuffer(allocator, &uBufferCI, &uBufferAllocCI,
                        &shaderDataBuffers[i].buffer,
                        &shaderDataBuffers[i].allocation,
                        &shaderDataBuffers[i].allocationInfo));
    VkBufferDeviceAddressInfo uBufferBdaInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = shaderDataBuffers[i].buffer};
    shaderDataBuffers[i].deviceAddress =
        vkGetBufferDeviceAddress(device, &uBufferBdaInfo);
  }
  // Sync objects
  VkSemaphoreCreateInfo semaphoreCI{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  VkFenceCreateInfo fenceCI{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                            .flags = VK_FENCE_CREATE_SIGNALED_BIT};
  for (auto i = 0; i < maxFramesInFlight; i++) {
    chk(vkCreateFence(device, &fenceCI, nullptr, &fences[i]));
    chk(vkCreateSemaphore(device, &semaphoreCI, nullptr,
                          &presentSemaphores[i]));
  }
  renderSemaphores.resize(swapchainImages.size());
  for (auto &semaphore : renderSemaphores) {
    chk(vkCreateSemaphore(device, &semaphoreCI, nullptr, &semaphore));
  }
  // Command pool
  VkCommandPoolCreateInfo commandPoolCI{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = queueFamily};
  chk(vkCreateCommandPool(device, &commandPoolCI, nullptr, &commandPool));
  VkCommandBufferAllocateInfo cbAllocCI{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = commandPool,
      .commandBufferCount = maxFramesInFlight};
  chk(vkAllocateCommandBuffers(device, &cbAllocCI, commandBuffers.data()));
  // Texture images
  std::vector<VkDescriptorImageInfo> textureDescriptors{};
  for (auto i = 0; i < textures.size(); i++) {
    ktxTexture *ktxTexture{nullptr};
    std::string filename = "assets/suzanne" + std::to_string(i) + ".ktx";
    ktxTexture_CreateFromNamedFile(
        filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
    VkImageCreateInfo texImgCI{.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                               .imageType = VK_IMAGE_TYPE_2D,
                               .format = ktxTexture_GetVkFormat(ktxTexture),
                               .extent = {.width = ktxTexture->baseWidth,
                                          .height = ktxTexture->baseHeight,
                                          .depth = 1},
                               .mipLevels = ktxTexture->numLevels,
                               .arrayLayers = 1,
                               .samples = VK_SAMPLE_COUNT_1_BIT,
                               .tiling = VK_IMAGE_TILING_OPTIMAL,
                               .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                        VK_IMAGE_USAGE_SAMPLED_BIT,
                               .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};
    VmaAllocationCreateInfo texImageAllocCI{.usage = VMA_MEMORY_USAGE_AUTO};
    chk(vmaCreateImage(allocator, &texImgCI, &texImageAllocCI,
                       &textures[i].image, &textures[i].allocation, nullptr));
    VkImageViewCreateInfo texVewCI{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = textures[i].image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = texImgCI.format,
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                             .levelCount = ktxTexture->numLevels,
                             .layerCount = 1}};
    chk(vkCreateImageView(device, &texVewCI, nullptr, &textures[i].view));
    // Upload
    VkBuffer imgSrcBuffer{};
    VmaAllocation imgSrcAllocation{};
    VkBufferCreateInfo imgSrcBufferCI{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = (uint32_t)ktxTexture->dataSize,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT};
    VmaAllocationCreateInfo imgSrcAllocCI{
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                 VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO};
    VmaAllocationInfo imgSrcAllocInfo{};
    chk(vmaCreateBuffer(allocator, &imgSrcBufferCI, &imgSrcAllocCI,
                        &imgSrcBuffer, &imgSrcAllocation, &imgSrcAllocInfo));
    memcpy(imgSrcAllocInfo.pMappedData, ktxTexture->pData,
           ktxTexture->dataSize);
    VkFenceCreateInfo fenceOneTimeCI{.sType =
                                         VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VkFence fenceOneTime{};
    chk(vkCreateFence(device, &fenceOneTimeCI, nullptr, &fenceOneTime));
    VkCommandBuffer cbOneTime{};
    VkCommandBufferAllocateInfo cbOneTimeAI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .commandBufferCount = 1};
    chk(vkAllocateCommandBuffers(device, &cbOneTimeAI, &cbOneTime));
    VkCommandBufferBeginInfo cbOneTimeBI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
    chk(vkBeginCommandBuffer(cbOneTime, &cbOneTimeBI));
    VkImageMemoryBarrier2 barrierTexImage{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
        .srcAccessMask = VK_ACCESS_2_NONE,
        .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .image = textures[i].image,
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                             .levelCount = ktxTexture->numLevels,
                             .layerCount = 1}};
    VkDependencyInfo barrierTexInfo{.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                    .imageMemoryBarrierCount = 1,
                                    .pImageMemoryBarriers = &barrierTexImage};
    vkCmdPipelineBarrier2(cbOneTime, &barrierTexInfo);
    std::vector<VkBufferImageCopy> copyRegions{};
    for (auto j = 0; j < ktxTexture->numLevels; j++) {
      ktx_size_t mipOffset{0};
      KTX_error_code ret =
          ktxTexture_GetImageOffset(ktxTexture, j, 0, 0, &mipOffset);
      copyRegions.push_back({
          .bufferOffset = mipOffset,
          .imageSubresource{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                            .mipLevel = (uint32_t)j,
                            .layerCount = 1},
          .imageExtent{.width = ktxTexture->baseWidth >> j,
                       .height = ktxTexture->baseHeight >> j,
                       .depth = 1},
      });
    }
    vkCmdCopyBufferToImage(cbOneTime, imgSrcBuffer, textures[i].image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           static_cast<uint32_t>(copyRegions.size()),
                           copyRegions.data());
    VkImageMemoryBarrier2 barrierTexRead{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
        .image = textures[i].image,
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                             .levelCount = ktxTexture->numLevels,
                             .layerCount = 1}};
    barrierTexInfo.pImageMemoryBarriers = &barrierTexRead;
    vkCmdPipelineBarrier2(cbOneTime, &barrierTexInfo);
    chk(vkEndCommandBuffer(cbOneTime));
    VkSubmitInfo oneTimeSI{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                           .commandBufferCount = 1,
                           .pCommandBuffers = &cbOneTime};
    chk(vkQueueSubmit(queue, 1, &oneTimeSI, fenceOneTime));
    chk(vkWaitForFences(device, 1, &fenceOneTime, VK_TRUE, UINT64_MAX));
    vkDestroyFence(device, fenceOneTime, nullptr);
    vmaDestroyBuffer(allocator, imgSrcBuffer, imgSrcAllocation);
    // Sampler
    VkSamplerCreateInfo samplerCI{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = 8.0f,
        .maxLod = (float)ktxTexture->numLevels,
    };
    chk(vkCreateSampler(device, &samplerCI, nullptr, &textures[i].sampler));
    ktxTexture_Destroy(ktxTexture);
    textureDescriptors.push_back(
        {.sampler = textures[i].sampler,
         .imageView = textures[i].view,
         .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL});
  }
  // Descriptor (indexing)
  VkDescriptorBindingFlags descVariableFlag{
      VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT};
  VkDescriptorSetLayoutBindingFlagsCreateInfo descBindingFlags{
      .sType =
          VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
      .bindingCount = 1,
      .pBindingFlags = &descVariableFlag};
  VkDescriptorSetLayoutBinding descLayoutBindingTex{
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = static_cast<uint32_t>(textures.size()),
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT};
  VkDescriptorSetLayoutCreateInfo descLayoutTexCI{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = &descBindingFlags,
      .bindingCount = 1,
      .pBindings = &descLayoutBindingTex};
  chk(vkCreateDescriptorSetLayout(device, &descLayoutTexCI, nullptr,
                                  &descriptorSetLayoutTex));
  VkDescriptorPoolSize poolSize{
      .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = static_cast<uint32_t>(textures.size())};
  VkDescriptorPoolCreateInfo descPoolCI{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = 1,
      .poolSizeCount = 1,
      .pPoolSizes = &poolSize};
  chk(vkCreateDescriptorPool(device, &descPoolCI, nullptr, &descriptorPool));
  uint32_t variableDescCount{static_cast<uint32_t>(textures.size())};
  VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescCountAI{
      .sType =
          VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT,
      .descriptorSetCount = 1,
      .pDescriptorCounts = &variableDescCount};
  VkDescriptorSetAllocateInfo texDescSetAlloc{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .pNext = &variableDescCountAI,
      .descriptorPool = descriptorPool,
      .descriptorSetCount = 1,
      .pSetLayouts = &descriptorSetLayoutTex};
  chk(vkAllocateDescriptorSets(device, &texDescSetAlloc, &descriptorSetTex));
  VkWriteDescriptorSet writeDescSet{
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = descriptorSetTex,
      .dstBinding = 0,
      .descriptorCount = static_cast<uint32_t>(textureDescriptors.size()),
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .pImageInfo = textureDescriptors.data()};
  vkUpdateDescriptorSets(device, 1, &writeDescSet, 0, nullptr);
  // Initialize Slang shader compiler
  slang::createGlobalSession(slangGlobalSession.writeRef());
  auto slangTargets{std::to_array<slang::TargetDesc>(
      {{.format{SLANG_SPIRV},
        .profile{slangGlobalSession->findProfile("spirv_1_4")}}})};
  auto slangOptions{std::to_array<slang::CompilerOptionEntry>(
      {{slang::CompilerOptionName::EmitSpirvDirectly,
        {slang::CompilerOptionValueKind::Int, 1}}})};
  slang::SessionDesc slangSessionDesc{
      .targets{slangTargets.data()},
      .targetCount{SlangInt(slangTargets.size())},
      .defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
      .compilerOptionEntries{slangOptions.data()},
      .compilerOptionEntryCount{uint32_t(slangOptions.size())}};
  // Load shader
  Slang::ComPtr<slang::ISession> slangSession;
  slangGlobalSession->createSession(slangSessionDesc, slangSession.writeRef());
  Slang::ComPtr<slang::IModule> slangModule{slangSession->loadModuleFromSource(
      "triangle", "assets/shader.slang", nullptr, nullptr)};
  Slang::ComPtr<ISlangBlob> spirv;
  slangModule->getTargetCode(0, spirv.writeRef());
  VkShaderModuleCreateInfo shaderModuleCI{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = spirv->getBufferSize(),
      .pCode = (uint32_t *)spirv->getBufferPointer()};
  chk(vkCreateShaderModule(device, &shaderModuleCI, nullptr,
                           &ctx->shaderModule));
  // Pipeline
  VkPushConstantRange pushConstantRange{.stageFlags =
                                            VK_SHADER_STAGE_VERTEX_BIT,
                                        .size = sizeof(VkDeviceAddress)};
  VkPipelineLayoutCreateInfo pipelineLayoutCI{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &descriptorSetLayoutTex,
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &pushConstantRange};
  chk(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr,
                             &pipelineLayout));
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages{
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_VERTEX_BIT,
       .module = ctx->shaderModule,
       .pName = "main"},
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
       .module = ctx->shaderModule,
       .pName = "main"}};
  VkVertexInputBindingDescription vertexBinding{
      .binding = 0,
      .stride = sizeof(Vertex),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
  std::vector<VkVertexInputAttributeDescription> vertexAttributes{
      {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT},
      {.location = 1,
       .binding = 0,
       .format = VK_FORMAT_R32G32B32_SFLOAT,
       .offset = offsetof(Vertex, normal)},
      {.location = 2,
       .binding = 0,
       .format = VK_FORMAT_R32G32_SFLOAT,
       .offset = offsetof(Vertex, uv)},
  };
  VkPipelineVertexInputStateCreateInfo vertexInputState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &vertexBinding,
      .vertexAttributeDescriptionCount =
          static_cast<uint32_t>(vertexAttributes.size()),
      .pVertexAttributeDescriptions = vertexAttributes.data(),
  };
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
  std::vector<VkDynamicState> dynamicStates{VK_DYNAMIC_STATE_VIEWPORT,
                                            VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = 2,
      .pDynamicStates = dynamicStates.data()};
  VkPipelineViewportStateCreateInfo viewportState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .scissorCount = 1};
  VkPipelineRasterizationStateCreateInfo rasterizationState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .lineWidth = 1.0f};
  VkPipelineMultisampleStateCreateInfo multisampleState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};
  VkPipelineDepthStencilStateCreateInfo depthStencilState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = VK_TRUE,
      .depthWriteEnable = VK_TRUE,
      .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL};
  VkPipelineColorBlendAttachmentState blendAttachment{.colorWriteMask = 0xF};
  VkPipelineColorBlendStateCreateInfo colorBlendState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments = &blendAttachment};
  VkPipelineRenderingCreateInfo renderingCI{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &ctx->imageFormat,
      .depthAttachmentFormat = ctx->depthFormat};
  VkGraphicsPipelineCreateInfo pipelineCI{
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = &renderingCI,
      .stageCount = 2,
      .pStages = shaderStages.data(),
      .pVertexInputState = &vertexInputState,
      .pInputAssemblyState = &inputAssemblyState,
      .pViewportState = &viewportState,
      .pRasterizationState = &rasterizationState,
      .pMultisampleState = &multisampleState,
      .pDepthStencilState = &depthStencilState,
      .pColorBlendState = &colorBlendState,
      .pDynamicState = &dynamicState,
      .layout = pipelineLayout};
  chk(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr,
                                &pipeline));
}

void drawFrame(vulkanContext *ctx, uint64_t lastTime, RenderQueue *rq,
               ShaderData *shaderData) {

  // Sync
  chk(vkWaitForFences(device, 1, &fences[frameIndex], true, UINT64_MAX));
  chk(vkResetFences(device, 1, &fences[frameIndex]));
  chkSwapchain(vkAcquireNextImageKHR(device, ctx->swapchain, UINT64_MAX,
                                     presentSemaphores[frameIndex],
                                     VK_NULL_HANDLE, &imageIndex),
               ctx->updateSwapchain);
  memcpy(shaderDataBuffers[frameIndex].allocationInfo.pMappedData, shaderData,
         sizeof(ShaderData));
  // Build command buffer
  auto cb = commandBuffers[frameIndex];
  chk(vkResetCommandBuffer(cb, 0));
  VkCommandBufferBeginInfo cbBI{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
  chk(vkBeginCommandBuffer(cb, &cbBI));

  // NOTE: Advanced syncing stuff Not relevant for now
  std::array<VkImageMemoryBarrier2, 2> outputBarriers{
      VkImageMemoryBarrier2{
          .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
          .pNext = nullptr,
          .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
          .srcAccessMask = 0,
          .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
          .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                           VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
          .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
          .image = swapchainImages[imageIndex],
          .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                            .levelCount = 1,
                            .layerCount = 1}},
      VkImageMemoryBarrier2{
          .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
          .srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
          .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
          .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
          .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
          .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
          .image = depthImage,
          .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT |
                                          VK_IMAGE_ASPECT_STENCIL_BIT,
                            .levelCount = 1,
                            .layerCount = 1}}};
  VkDependencyInfo barrierDependencyInfo{
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .imageMemoryBarrierCount = 2,
      .pImageMemoryBarriers = outputBarriers.data()};
  vkCmdPipelineBarrier2(cb, &barrierDependencyInfo);
  // NOTE: End of complicated sync stuff ^^

  // NOTE: Above syncs these. This one defines clear color.
  VkRenderingAttachmentInfo colorAttachmentInfo{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = swapchainImageViews[imageIndex],
      .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue{.color{0.33f, 0.5f, 0.33f, 255.0f}}};
  VkRenderingAttachmentInfo depthAttachmentInfo{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = depthImageView,
      .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .clearValue = {.depthStencil = {1.0f, 0}}};
  VkRenderingInfo renderingInfo{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea{.extent{.width = static_cast<uint32_t>(ctx->windowSize[0]),
                          .height = static_cast<uint32_t>(ctx->windowSize[1])}},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachmentInfo,
      .pDepthAttachment = &depthAttachmentInfo};
  vkCmdBeginRendering(cb, &renderingInfo);
  // NOTE: x,y moves the viewport from top left.
  // width, height determines size from x,y
  // maxdepth defines the scale for depth buffer
  //  --------------------------------------
  //  |                                    |
  //  |   *                  *             |
  //  |                                    |
  //  |                                    |
  //  |   *                  *             |
  //  |                                    |
  //  --------------------------------------

  VkViewport vp{.x = 0,
                .y = 0,
                .width = static_cast<float>(ctx->windowSize[0]),
                .height = static_cast<float>(ctx->windowSize[1]),
                .minDepth = 0.0f,
                .maxDepth = 1.0f};
  // NOTE: just adds viewport to command buffer
  vkCmdSetViewport(cb, 0, 1, &vp);
  // NOTE: scissor cuts from viewport not effecting the model scale etc.
  //  Just hard cut. Monki stay same size, not showing other monki
  //   --------------------------------------
  //   |                                    |
  //   |   *         |         *            |
  //   |      left   |   cut                |
  //   |   -----------                      |
  //   |      cut       cut                 |
  //   |   *                  *             |
  //   --------------------------------------
  VkRect2D scissor{
      .extent{.width = static_cast<uint32_t>(ctx->windowSize[0]),
              .height = static_cast<uint32_t>(ctx->windowSize[1])}};
  vkCmdSetScissor(cb, 0, 1, &scissor);

  // Rendering — draw ctx->mesh directly
  // TODO: restore renderQueue loop once MeshGPUData/meshTable is wired up
  // for (int i = 0; i < rq->itemCount; i++) {
  //   RenderItem *item = &rq->items[i];
  //   MeshGPUData *gpu = &meshTable[item->mesh];
  //   if (pipeline != lastPipeline) {
  //     vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
  //     vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
  //                             pipelineLayout, 0, 1, &descriptorSetTex, 0,
  //                             nullptr);
  //     lastPipeline = pipeline;
  //   }
  //   VkDeviceSize vOffset = 0;
  //   vkCmdBindVertexBuffers(cb, 0, 1, &gpu->vertexBuffer, &vOffset);
  //   vkCmdBindIndexBuffer(cb, gpu->indexBuffer, gpu->indexOffset,
  //   VK_INDEX_TYPE_UINT16); vkCmdPushConstants(cb, pipelineLayout,
  //   VK_SHADER_STAGE_VERTEX_BIT, 0,
  //                      sizeof(VkDeviceAddress),
  //                      &shaderDataBuffers[frameIndex].deviceAddress);
  //   vkCmdDrawIndexed(cb, gpu->indexCount, item->instanceCount, 0, 0, 0);
  // }
  vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
  vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                          0, 1, &descriptorSetTex, 0, nullptr);
  VkDeviceSize vOffset = 0;
  vkCmdBindVertexBuffers(cb, 0, 1, &ctx->mesh.vertices.buf, &vOffset);
  vkCmdBindIndexBuffer(cb, ctx->mesh.vertices.buf, ctx->mesh.vertices.size,
                       VK_INDEX_TYPE_UINT16);
  vkCmdPushConstants(cb, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                     sizeof(VkDeviceAddress),
                     &shaderDataBuffers[frameIndex].deviceAddress);
  vkCmdDrawIndexed(cb, ctx->mesh.indexCount, MONKI_COUNT, 0, 0, 0);
  vkCmdEndRendering(cb);

  // Syncing?
  VkImageMemoryBarrier2 barrierPresent{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstAccessMask = 0,
      .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      .image = swapchainImages[imageIndex],
      .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .levelCount = 1,
                        .layerCount = 1}};
  VkDependencyInfo barrierPresentDependencyInfo{
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &barrierPresent};
  vkCmdPipelineBarrier2(cb, &barrierPresentDependencyInfo);
  chk(vkEndCommandBuffer(cb));
  // Submit to graphics queue
  VkPipelineStageFlags waitStages =
      VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSubmitInfo submitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &presentSemaphores[frameIndex],
      .pWaitDstStageMask = &waitStages,
      .commandBufferCount = 1,
      .pCommandBuffers = &cb,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &renderSemaphores[imageIndex],
  };
  chk(vkQueueSubmit(queue, 1, &submitInfo, fences[frameIndex]));
  frameIndex = (frameIndex + 1) % maxFramesInFlight;
  VkPresentInfoKHR presentInfo{.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                               .waitSemaphoreCount = 1,
                               .pWaitSemaphores = &renderSemaphores[imageIndex],
                               .swapchainCount = 1,
                               .pSwapchains = &ctx->swapchain,
                               .pImageIndices = &imageIndex};
  chkSwapchain(vkQueuePresentKHR(queue, &presentInfo), ctx->updateSwapchain);
  if (ctx->updateSwapchain) {
    chk(SDL_GetWindowSize(ctx->window, &ctx->windowSize[0],
                          &ctx->windowSize[1]));
    ctx->updateSwapchain = false;
    chk(vkDeviceWaitIdle(device));
    chk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        ctx->devices[ctx->deviceIndex], surface, &ctx->surfaceCaps));
    ctx->swapchainCI.oldSwapchain = ctx->swapchain;
    ctx->swapchainCI.imageExtent = {
        .width = static_cast<uint32_t>(ctx->windowSize[0]),
        .height = static_cast<uint32_t>(ctx->windowSize[1])};
    chk(vkCreateSwapchainKHR(device, &ctx->swapchainCI, nullptr,
                             &ctx->swapchain));
    for (auto i = 0; i < ctx->imageCount; i++) {
      vkDestroyImageView(device, swapchainImageViews[i], nullptr);
    }
    chk(vkGetSwapchainImagesKHR(device, ctx->swapchain, &ctx->imageCount,
                                nullptr));
    swapchainImages.resize(ctx->imageCount);
    chk(vkGetSwapchainImagesKHR(device, ctx->swapchain, &ctx->imageCount,
                                swapchainImages.data()));
    swapchainImageViews.resize(ctx->imageCount);
    for (auto i = 0; i < ctx->imageCount; i++) {
      VkImageViewCreateInfo viewCI{
          .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
          .image = swapchainImages[i],
          .viewType = VK_IMAGE_VIEW_TYPE_2D,
          .format = ctx->imageFormat,
          .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                               .levelCount = 1,
                               .layerCount = 1}};
      chk(vkCreateImageView(device, &viewCI, nullptr, &swapchainImageViews[i]));
    }
    vkDestroySwapchainKHR(device, ctx->swapchainCI.oldSwapchain, nullptr);
    vmaDestroyImage(allocator, depthImage, depthImageAllocation);
    vkDestroyImageView(device, depthImageView, nullptr);
    ctx->depthImageCI.extent = {
        .width = static_cast<uint32_t>(ctx->windowSize[0]),
        .height = static_cast<uint32_t>(ctx->windowSize[1]),
        .depth = 1};
    VmaAllocationCreateInfo allocCI{
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO};
    chk(vmaCreateImage(allocator, &ctx->depthImageCI, &allocCI, &depthImage,
                       &depthImageAllocation, nullptr));
    VkImageViewCreateInfo viewCI{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = depthImage,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = ctx->depthFormat,
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                             .levelCount = 1,
                             .layerCount = 1}};
    chk(vkCreateImageView(device, &viewCI, nullptr, &depthImageView));
  }
}
void pollEvents(vulkanContext *ctx, Input *input) {
  input->mouseDeltaX = 0;
  input->mouseDeltaY = 0;
  input->mouseWheel = 0;

  for (SDL_Event event; SDL_PollEvent(&event);) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
      ctx->quit = true;
      break;
    case SDL_EVENT_KEY_DOWN:
      input->keys[event.key.scancode] = true;
      break;
    case SDL_EVENT_KEY_UP:
      input->keys[event.key.scancode] = false;
      break;
    case SDL_EVENT_MOUSE_MOTION:
      input->mouseDeltaX += event.motion.xrel;
      input->mouseDeltaY += event.motion.yrel;
      input->mouseX = event.motion.x;
      input->mouseY = event.motion.y;
      break;
    case SDL_EVENT_MOUSE_WHEEL:
      input->mouseWheel += event.wheel.y;
      break;
    case SDL_EVENT_WINDOW_RESIZED:
      ctx->updateSwapchain = true;
      break;
    }
  }
}
// void pollEvents(vulkanContext *ctx, uint64_t lastTime, ShaderData
// *shaderData) {
//   // Event polling
//   float elapsedTime{(SDL_GetTicks() - lastTime) / 1000.0f};
//   lastTime = SDL_GetTicks();
//   for (SDL_Event event; SDL_PollEvent(&event);) {
//     if (event.type == SDL_EVENT_QUIT) {
//       ctx->quit = true;
//       break;
//     }
//     if (event.type == SDL_EVENT_MOUSE_MOTION) {
//       // if (event.button.button == SDL_BUTTON_LEFT) {
//       //   shaderData->objectRotations[shaderData->selected].x -=
//       //       (float)event.motion.yrel * elapsedTime;
//       //   shaderData->objectRotations[shaderData->selected].y +=
//       //       (float)event.motion.xrel * elapsedTime;
//       // }
//     }
//     // if (event.type == SDL_EVENT_MOUSE_WHEEL) {
//     //   camPos.z += (float)event.wheel.y * elapsedTime * 10.0f;
//     // }
//     if (event.type == SDL_EVENT_KEY_DOWN) {
//       // if (event.key.key == SDLK_PLUS || event.key.key == SDLK_KP_PLUS) {
//       //   shaderData.selected =
//       //       (shaderData.selected < 2) ? shaderData.selected + 1 : 0;
//       // }
//       // if (event.key.key == SDLK_MINUS || event.key.key == SDLK_KP_MINUS) {
//       //   shaderData.selected =
//       //       (shaderData.selected > 0) ? shaderData.selected - 1 : 2;
//       // }
//       if (event.key.key == SDLK_R) {
//         // Overwrite mesh buffer with a triangle (3 verts, 3 indices)
//         struct SimpleVertex {
//           float pos[3];
//           float normal[3];
//           float uv[2];
//         };
//         SimpleVertex triVerts[3] = {
//             {{0.0f, 1.0f, 0.0f}, {0, 0, 1}, {0.5f, 1.0f}},
//             {{-1.0f, -1.0f, 0.0f}, {0, 0, 1}, {0.0f, 0.0f}},
//             {{1.0f, -1.0f, 0.0f}, {0, 0, 1}, {1.0f, 0.0f}}};
//         uint16_t triIndices[3] = {0, 1, 2};
//         // Write to mapped buffer
//         void *mapped = ctx->mesh.vertices.allocationInfo.pMappedData;
//         memcpy(mapped, triVerts, sizeof(triVerts));
//         memcpy((char *)mapped + sizeof(triVerts), triIndices,
//                sizeof(triIndices));
//         ctx->mesh.vertices.size = sizeof(triVerts);
//         ctx->mesh.indicesSize = sizeof(triIndices);
//         ctx->mesh.indexCount = 3;
//         log_info("Mesh buffer overwritten with triangle");
//       }
//     }
//     // Window resize
//     if (event.type == SDL_EVENT_WINDOW_RESIZED) {
//       ctx->updateSwapchain = true;
//     }
//   }
// }
void destroy(std::unique_ptr<vulkanContext> ctx) {
  // Tear down
  chk(vkDeviceWaitIdle(device));
  for (auto i = 0; i < maxFramesInFlight; i++) {
    vkDestroyFence(device, fences[i], nullptr);
    vkDestroySemaphore(device, presentSemaphores[i], nullptr);
    vmaDestroyBuffer(allocator, shaderDataBuffers[i].buffer,
                     shaderDataBuffers[i].allocation);
  }
  for (auto i = 0; i < renderSemaphores.size(); i++) {
    vkDestroySemaphore(device, renderSemaphores[i], nullptr);
  }
  vmaDestroyImage(allocator, depthImage, depthImageAllocation);
  vkDestroyImageView(device, depthImageView, nullptr);
  for (auto i = 0; i < swapchainImageViews.size(); i++) {
    vkDestroyImageView(device, swapchainImageViews[i], nullptr);
  }
  vmaDestroyBuffer(allocator, ctx->mesh.vertices.buf,
                   ctx->mesh.vertices.allocation);
  for (auto i = 0; i < textures.size(); i++) {
    vkDestroyImageView(device, textures[i].view, nullptr);
    vkDestroySampler(device, textures[i].sampler, nullptr);
    vmaDestroyImage(allocator, textures[i].image, textures[i].allocation);
  }
  vkDestroyDescriptorSetLayout(device, descriptorSetLayoutTex, nullptr);
  vkDestroyDescriptorPool(device, descriptorPool, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  vkDestroyPipeline(device, pipeline, nullptr);
  vkDestroySwapchainKHR(device, ctx->swapchain, nullptr);
  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyCommandPool(device, commandPool, nullptr);
  vkDestroyShaderModule(device, ctx->shaderModule, nullptr);
  vmaDestroyAllocator(allocator);
  SDL_DestroyWindow(ctx->window);
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
  SDL_Quit();
  vkDestroyDevice(device, nullptr);
  vkDestroyInstance(instance, nullptr);
}
