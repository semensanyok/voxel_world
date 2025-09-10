#ifndef SW_RENDERER_CONTEXT
#define SW_RENDERER_CONTEXT

#include "gpu_structs.h"
#include "settings_global.h"
#include "vw_constants.h"
#include "vw_utils.h"
#include <SDL.h>
#include <SDL_stdinc.h>
#include <SDL_vulkan.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>

#define STR(x) #x
#define XSTR(x) STR(x)

const int MAX_FRAMES_IN_FLIGHT = 2;

struct QueueFamily {
  uint32_t id = 0;
  // index to start from when calling vkGetDeviceQueue
  uint32_t start_queue_num = 0;
  uint32_t num_queues = 0;
  VkQueueFlags type;
  bool has_available() { return num_queues > 0; }
  void advance_to_next_queue() {
    start_queue_num++;
    num_queues--;
  }
};
struct QueueFamilyIndices {
  std::optional<QueueFamily> graphicsFamily;
  std::optional<QueueFamily> presentFamily;
  // Some GPU announce dedicated transfer queues, utilising DMA
  // to asynchronously transfer data between host and device memory on discrete
  // GPUs, so transfers can be done concurrently with independent
  // graphics/compute operations.
  // Graphics and present also support transfer. Consider them last, if no
  // dedicated found.
  // Queue Families with only VK_QUEUE_TRANSFER_BIT are usually for using DMA.
  // single queue is ok, not a bottleneck. (see Vulkan_optimizations_2.md: Godot
  // uses single queue; Transfer speed is limited by memory bandwidth, not queue
  // submissions; Lock held for microseconds, not milliseconds)
  std::optional<QueueFamily> transferFamily;
  std::optional<QueueFamily> computeFamily;
  bool isCompleteGraphics() {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

struct StagingBuffer {
  VkBuffer buffer;
  VkDeviceMemory memory;
  void *data;
  size_t size = BufferSettings::STAGING_BUFFER_SIZE;
  size_t offset = 0; // Current write position

  // staging buffer per thread for simplicity. switch to shared buffer in the
  // future with offsets
  // ... or no. false sharing?
  // consider allocate on a first write, resize when needed.
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  // BufferOffsets targetBufferOffsets;
};

struct TransferWorker {
  VkCommandPool commandPool;
  std::vector<VkCommandBuffer> commandBuffers;
  // 1 buffer per thread, to avoid contention
  StagingBuffer stagingBuffer;
  std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> transferSemaphores;
};

class RendererContext {

private:
  uint32_t currentFrame = 0;

  SDL_Window *window;

  VkInstance instance;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;

  VkSurfaceKHR surface;

  // - Queue Families with only VK_QUEUE_TRANSFER_BIT are usually for using DMA
  // to asynchronously transfer data between host and device memory on discrete
  // GPUs, so transfers can be done concurrently with independent
  // graphics/compute operations.
  // - VK_QUEUE_GRAPHICS_BIT and VK_QUEUE_COMPUTE_BIT can always implicitly
  // accept VK_QUEUE_TRANSFER_BIT commands.
  VkQueue graphicsQueue;
  VkQueue presentQueue;
  VkQueue transferQueue;

  VkCommandPool drawCommandPool;
  VkCommandBuffer drawCommandBuffer;
  std::vector<TransferWorker> transferWorkers;
  std::array<std::vector<VkSemaphore>, MAX_FRAMES_IN_FLIGHT> transferSemaphores;

  VkQueue computeQueue;
  VkCommandPool computeCommandPool;
  VkCommandBuffer computeCommandBuffer;

  VkSwapchainKHR swapChain;
  std::vector<VkImage> swapChainImages;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;
  std::vector<VkImageView> swapChainImageViews;

  VkRenderPass renderPass;
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;

  VkPipelineLayout computePipelineLayout;
  VkPipeline computePipeline;

  std::vector<VkFramebuffer> swapChainFramebuffers;

  /* https://developer.nvidia.com/vulkan-memory-management
   * Recommends using same buffer with offsets for vert/ind/uniform.
   *  For Buffer memory we recommend making use of the offset mechanism the API
   *  provides. Just like in OpenGL, Vulkan allows to binding a range of a
   *  buffer. The benefit is that we avoid CPU memory costs for lots of tiny
   *  buffers, as well as cache misses by using just the same buffer object and
   *  varying the offset.
   */
  VkBuffer deviceBuffer;
  VkDeviceMemory deviceBufferMemory;
  BufferOffsets deviceBufferOffsets;

  VkBuffer computeBuffer;
  VkDeviceMemory computeBufferMemory;

  QueueFamilyIndices queueFamilyIndices;

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
  BufferOffsets buffer(MeshLoadData &mesh_load_data) {}
  void postDraw() {
    vkResetCommandPool(device, drawCommandPool, NULL);
    for (auto &worker : transferWorkers) {
      vkResetCommandPool(device, worker.commandPool, NULL);
    }
  }

  void destroyStagingBuffer(StagingBuffer *staging);

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
  StagingBuffer* createStagingBuffer();
  QueueFamilyIndices
  findQueueFamilies(VkPhysicalDevice &physical_device_candidate);
  void setTransferFamily(std::vector<QueueFamily> &transferFamilies,
                         std::vector<QueueFamily> &graphicsFamilies,
                         std::vector<QueueFamily> &computeFamilies,
                         QueueFamilyIndices &indices);
  void getTransferQueue(
      bool &has_transfer, std::vector<QueueFamily>::iterator &transfer_iter,
      std::vector<QueueFamily> &transferFamilies, bool &has_graphics,
      std::vector<QueueFamily>::iterator &graphics_iter,
      std::vector<QueueFamily> &graphicsFamilies, bool &has_compute,
      std::vector<QueueFamily>::iterator &compute_iter,
      std::vector<QueueFamily> &computeFamilies, int &transfer_family_index,
      const int NUM_TRANSFER_QUEUES,
      std::array<QueueFamily, 1Ui64> &transferFamily, bool &init);
  bool isDeviceSuitable(VkPhysicalDevice &device);
  int rateDeviceSuitability(VkPhysicalDeviceProperties &deviceProperties,
                            VkPhysicalDeviceFeatures &deviceFeatures);
  void pickPhysicalDevice();
  void createLogicalDevice();
  void createSurface();
  std::vector<VkDeviceQueueCreateInfo>
  getPhysicalDeviceQueues(QueueFamilyIndices &indices);
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
  void createComputePipeline();
  VkShaderModule createShaderModule(const std::vector<char> &code);
  void createRenderPass();
  void createFramebuffers();
  VkCommandPool createCommandPool(VkCommandPoolCreateFlagBits &flags,
                                  uint32_t queueFamilyIndex);
  VkCommandPool createCommandPoolTransient(uint32_t queueFamilyIndex);
  VkCommandPool createCommandPoolPersistent(uint32_t queueFamilyIndex);
  std::vector<VkCommandBuffer>
  createCommandBuffer(VkCommandPool commandPool,
                      int numBuffers = MAX_FRAMES_IN_FLIGHT);
  void recordCommandBufferDraw(VkCommandBuffer commandBuffer,
                               uint32_t imageIndex);
  void createSyncObjects();
  void createDrawBuffer();
  void createComputeBuffer();
  void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties, VkBuffer &buffer,
                    VkDeviceMemory &bufferMemory);
  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
  uint32_t findMemoryType(uint32_t typeFilter,
                          VkMemoryPropertyFlags properties);
  std::vector<char> readShaderFile(const char *filename);
};

#endif
