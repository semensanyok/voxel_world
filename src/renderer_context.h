#ifndef SW_RENDERER_CONTEXT
#define SW_RENDERER_CONTEXT

#include "gpu_structs.h"
#include "settings_global.h"
#include "vw_utils.h"
#include <SDL.h>
#include <SDL_stdinc.h>
#include <SDL_vulkan.h>
#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>

#define STR(x) #x
#define XSTR(x) STR(x)

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
  const int MAX_FRAMES_IN_FLIGHT = 2;
  uint32_t currentFrame = 0;

  SDL_Window *window;

  VkInstance instance;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;

  VkSurfaceKHR surface;

  // TODO: transfer queue, to not batch/block transfer with drawing commands
  // https://vulkan-tutorial.com/Vertex_buffers/Staging_buffer
  /*
   Transfer queue
The buffer copy command requires a queue family that supports transfer
operations, which is indicated using VK_QUEUE_TRANSFER_BIT. The good news is
that any queue family with VK_QUEUE_GRAPHICS_BIT or VK_QUEUE_COMPUTE_BIT
capabilities already implicitly support VK_QUEUE_TRANSFER_BIT operations. The
implementation is not required to explicitly list it in queueFlags in those
cases.

If you like a challenge, then you can still try to use a different queue family
specifically for transfer operations. It will require you to make the following
modifications to your program:

- Modify QueueFamilyIndices and findQueueFamilies to explicitly look for a queue
family with the VK_QUEUE_TRANSFER_BIT bit, but not the VK_QUEUE_GRAPHICS_BIT.
- Modify createLogicalDevice
to request a handle to the transfer queue
- Create a second command pool
for command buffers that are submitted on the transfer queue family
- Change the sharingMode
of resources to be VK_SHARING_MODE_CONCURRENT and
specify both the graphics and transfer queue families
- Submit any transfer commands like vkCmdCopyBuffer
(which we'll be using in this chapter) to the
transfer queue instead of the graphics queue It's a bit of work, but it'll teach
you a lot about how resources are shared between queue families.
  */
  VkQueue graphicsQueue;
  VkQueue presentQueue;

  VkSwapchainKHR swapChain;
  std::vector<VkImage> swapChainImages;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;
  std::vector<VkImageView> swapChainImageViews;

  VkRenderPass renderPass;
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;

  std::vector<VkFramebuffer> swapChainFramebuffers;

  VkCommandPool commandPool;
  // Command buffers will be automatically freed when their command pool is
  // destroyed, so we don't need explicit cleanup.
  //
  // TODO: separate command buffer for transfer 1 time tasks with
  // VK_COMMAND_POOL_CREATE_TRANSIENT_BIT
  std::vector<VkCommandBuffer> commandBuffers;

  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  VkDebugUtilsMessengerEXT debugMessenger;

  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;

  bool framebufferResized = false;
  bool windowMinimizedOrHidden = false;

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
  int scr_width = GameSettings::SCR_WIDTH_INIT;
  int scr_height = GameSettings::SCR_HEIGHT_INIT;
  void init();
  void clear();
  void drawFrame();
  void recreateSwapChain();
  int windowCallback(SDL_Event *e);

private:
  // for test, to be removed ASAP.
  const std::vector<Vertex> vertices = {{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                        {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
                                        {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};

  void recreateWindow();
  void resizeWindow(SDL_Event *e);
  void minimizedWindow();
  void hiddenWindow();
  void restoredWindow();
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

  void cleanupSwapChain();
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
  void createImageViews();
  void createGraphicsPipeline();
  VkShaderModule createShaderModule(const std::vector<char> &code);
  void createRenderPass();
  void createFramebuffers();
  void createCommandPool();
  void createCommandBuffer();
  void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
  void createSyncObjects();
  void createVertexBuffer();
  void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties, VkBuffer &buffer,
                    VkDeviceMemory &bufferMemory);
  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
  uint32_t findMemoryType(uint32_t typeFilter,
                          VkMemoryPropertyFlags properties);
  std::vector<char> readShaderFile(const char *filename);
};

#endif
