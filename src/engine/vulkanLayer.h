#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <SDL3/SDL.h>

struct vulkanContext {
  VkBuffer vBuffer;
  VkDeviceSize vBufSize = 0;
  VkDeviceSize indexCount = 0;
  SDL_Window *window = nullptr;
  std::vector<VkPhysicalDevice> devices{};
  uint32_t deviceIndex = 0;
  VkSurfaceCapabilitiesKHR surfaceCaps{};
  VkSwapchainKHR swapchain{VK_NULL_HANDLE};
  VkSwapchainCreateInfoKHR swapchainCI{};
  uint32_t imageCount = 0;
  VkImageCreateInfo depthImageCI{};
  VkFormat depthFormat{VK_FORMAT_UNDEFINED};
};

void init(std::unique_ptr<vulkanContext> ctx);
void render(std::unique_ptr<vulkanContext> ctx);
void destroy(std::unique_ptr<vulkanContext> ctx);
