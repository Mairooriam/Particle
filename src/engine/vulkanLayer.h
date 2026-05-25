#pragma once
#include "internal/renderQue.h"
#include <cstdint>
#include <vulkan/vulkan.h>
#include <vector>
#include <SDL3/SDL.h>
#include "meshLoader.h"
#include "shader.h"
#include "shared.h"

struct vulkanContext {
  MeshBuffer mesh{};
  VkDeviceSize indexCount{0};
  SDL_Window *window = nullptr;
  ivec2 windowSize = {1280, 700};
  std::vector<VkPhysicalDevice> devices{};
  uint32_t deviceIndex{0};
  VkSurfaceCapabilitiesKHR surfaceCaps{};
  VkSwapchainKHR swapchain{VK_NULL_HANDLE};
  VkSwapchainCreateInfoKHR swapchainCI{};
  uint32_t imageCount{0};
  VkImageCreateInfo depthImageCI{};
  VkFormat depthFormat{VK_FORMAT_UNDEFINED};
  VkFormat imageFormat{VK_FORMAT_UNDEFINED};
  VkShaderModule shaderModule{};
  bool updateSwapchain{false};
  bool quit{false};
};

void init(vulkanContext *ctx, size_t shaderDataSize);
void drawFrame(vulkanContext *ctx, uint64_t lastTime, RenderQueue *rq,
               ShaderData *shaderData);
void pollEvents(vulkanContext *ctx, Input *input);
void destroy(vulkanContext *ctx);
