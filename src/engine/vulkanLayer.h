#pragma once
#include <cstdint>
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <SDL3/SDL.h>

struct vulkanContext {
  VkBuffer vBuffer;
  VkDeviceSize vBufSize{0};
  VkDeviceSize indexCount{0};
  SDL_Window *window = nullptr;
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

void init(std::unique_ptr<vulkanContext> &ctx);
void drawFrame(std::unique_ptr<vulkanContext> &ctx, uint64_t lastTime);
void pollEvents(std::unique_ptr<vulkanContext> &ctx, uint64_t lastTime);
void destroy(std::unique_ptr<vulkanContext> ctx);
