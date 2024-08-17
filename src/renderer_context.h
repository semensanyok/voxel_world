#ifndef SW_RENDERER_CONTEXT
#define SW_RENDERER_CONTEXT

#include <SDL.h>
#include <SDL_stdinc.h>
#include <SDL_vulkan.h>
#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;
  bool isComplete() {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

class RendererContext {

private:
  SDL_Window *window;

  VkInstance instance;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  VkQueue graphicsQueue;
  VkSurfaceKHR surface;
  VkQueue presentQueue;
  VkSwapchainKHR swapChain;

  VkDebugUtilsMessengerEXT debugMessenger;

#ifdef NDEBUG
  const bool enableValidationLayers = false;
#else
  const bool enableValidationLayers = true;
  const std::vector<const char *> validationLayers = {
      "VK_LAYER_KHRONOS_validation"};
#endif
  const std::vector<const char *> deviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};

public:
  void init();
  void clear();

private:
  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void *pUserData);

  VkResult CreateDebugUtilsMessengerEXT(
      const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
      const VkAllocationCallbacks *pAllocator,
      VkDebugUtilsMessengerEXT *pDebugMessenger);
  void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                     const VkAllocationCallbacks *pAllocator);
  void
  setupDebugCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &debugCreateInfo);

  QueueFamilyIndices
  findQueueFamilies(VkPhysicalDevice &physical_device_candidate);
  bool isDeviceSuitable(VkPhysicalDevice &device);
  int rateDeviceSuitability(VkPhysicalDeviceProperties &deviceProperties,
                            VkPhysicalDeviceFeatures &deviceFeatures);
  void pickPhysicalDevice();
  void createLogicalDevice();
  void createSurface();
  std::vector<VkDeviceQueueCreateInfo>
  get_physical_device_queues(QueueFamilyIndices &indices);
  void createInstance();
  bool checkValidationLayerSupport();
  bool checkDeviceExtensionSupport(VkPhysicalDevice &physical_device_candidate);
  SwapChainSupportDetails
  querySwapChainSupport(VkPhysicalDevice &physical_device_candidate);
  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats);
  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
  void createSwapChain();
};

#endif
