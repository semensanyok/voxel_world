#ifndef SW_RENDERER_CONTEXT
#define SW_RENDERER_CONTEXT

#include <SDL.h>
#include <SDL_vulkan.h>
#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
};

class RendererContext {

private:
  SDL_Window *window;

  VkInstance instance;
  VkPhysicalDevice physical_device = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT debugMessenger;

#ifdef NDEBUG
  const bool enableValidationLayers = false;
#else
  const bool enableValidationLayers = true;
  const std::vector<const char *> validationLayers = {
      "VK_LAYER_KHRONOS_validation"};
#endif
public:
  void init();
  void clear();
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
  void pick_physical_device();
  void create_logical_device(VkPhysicalDevice device);
  void create_vulkan_instance();
  bool checkValidationLayerSupport();
};

#endif
