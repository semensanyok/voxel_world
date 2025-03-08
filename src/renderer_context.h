#ifndef SW_RENDERER_CONTEXT
#define SW_RENDERER_CONTEXT

#include "gpu_structs.h"
#include "settings_global.h"
#include "vw_utils.h"
#include <SDL.h>
#include <SDL_stdinc.h>
#include <SDL_vulkan.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>

#define STR(x) #x
#define XSTR(x) STR(x)

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

//  You can only submit work to a VkQueue from one thread at a time, but
//  different threads can submit work to a different VkQueue simultaneously.
struct TransferQueue {
  uint32_t id;
  //  You can only submit work to a VkQueue from one thread at a time, but
  //  different threads can submit work to a different VkQueue simultaneously.
  std::atomic_bool in_use;
};

const int NUM_TRANSFER_QUEUES = 2;
// TODO: must be equal to the number of threads doing the recording.
//       Find out on the program init how many are required.
//       Could be less? Per thread recorded commands arrays flushed?
const int NUM_COMMAND_POOLS = 2;

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
  std::array<QueueFamily, NUM_TRANSFER_QUEUES> transferFamily;
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

// Command buffers will be automatically freed when their command pool is
// destroyed, so we don't need explicit cleanup.
//
// TODO: separate command buffer for transfer
// VK_COMMAND_POOL_CREATE_TRANSIENT_BIT
// `Vulkan_optimizations.md#1.2.1`
//
// pool per type of ops. dynamic culled draw/transfer with transient flag
// (full pool reset each frame), persistent UI layout - per command reset flag
// (https://docs.vulkan.org/spec/latest/chapters/cmdbuffers.html#commandbuffers-pools)
//
// Command pools are externally synchronized, meaning that a command pool must
// not be used concurrently in multiple threads. That includes use via recording
// commands on any command buffers allocated from the pool, as well as
// operations that allocate, free, and reset command buffers or the pool itself.
// It's not possible to record 2 command buffers from the same pool on different
// threads. Create pool per thread, even for queues from same family.
struct ThreadCommandPool {
  VkCommandPool commandPool;
  std::atomic_bool in_use;
  // commands resetted each frame, like dynamic draw or transfer from staging to
  // device buffer
  std::vector<VkCommandBuffer> transient_commands;
  // persistent commands, like non immediate gui
  std::vector<VkCommandBuffer> persistent_commands;
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

  // - Queue Families with only VK_QUEUE_TRANSFER_BIT are usually for using DMA
  // to asynchronously transfer data between host and device memory on discrete
  // GPUs, so transfers can be done concurrently with independent
  // graphics/compute operations.
  // - VK_QUEUE_GRAPHICS_BIT and VK_QUEUE_COMPUTE_BIT can always implicitly
  // accept VK_QUEUE_TRANSFER_BIT commands.
  std::array<TransferQueue, NUM_TRANSFER_QUEUES> transfer_queues;

  // idea is to asign transfer queue per thread and distribute transfer tasks
  // between capable threads.
  TransferQueue *GetAvailableTransferQueue() {
    for (auto &tq : transfer_queues) {
      bool expected = false;
      bool desired = true;
      if (tq.in_use.compare_exchange_strong(expected, desired)) {
        return &tq;
      }
    }
    return nullptr;
  }

  VkSwapchainKHR swapChain;
  std::vector<VkImage> swapChainImages;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;
  std::vector<VkImageView> swapChainImageViews;

  VkRenderPass renderPass;
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;

  std::vector<VkFramebuffer> swapChainFramebuffers;

  // TODO: delete 2 ThreadCommandPool variables. Replace with TransferWorker
  // alike (Godot)
  ThreadCommandPool dynamic_draw_commands[NUM_COMMAND_POOLS];
  ThreadCommandPool transfer_commands[NUM_COMMAND_POOLS];

  /* https://developer.nvidia.com/vulkan-memory-management
   * Recommends using same buffer with offsets for vert/ind/uniform.
   *  For Buffer memory we recommend making use of the offset mechanism the API
   *  provides. Just like in OpenGL, Vulkan allows to binding a range of a
   *  buffer. The benefit is that we avoid CPU memory costs for lots of tiny
   *  buffers, as well as cache misses by using just the same buffer object and
   *  varying the offset.
   */

  // VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, long term infrequent change? (my
  // system reports 9 gb) textures.
  VkBuffer deviceBuffer;
  VkDeviceMemory deviceBufferMemory;
  BufferOffsets deviceBufferOffsets;

  // TODO: each thread write its own staging buffer with memory_order_release
  // between frames transfer thread writes to device buffer, reading up to date
  // staging with memory_order_acquire.
  // thread id as index to vector with buf id.
  // task based parallelism, each HW thread will have it
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  BufferOffsets stagingBufferOffsets;

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
    for (ThreadCommandPool &p : dynamic_draw_commands) {
      vkResetCommandPool(device, p.commandPool, NULL);
    }
    for (ThreadCommandPool &p : transfer_commands) {
      vkResetCommandPool(device, p.commandPool, NULL);
    }
  }

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
