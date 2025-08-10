#include "renderer_context.h"
#include "gpu_structs.h"
#include "settings_global.h"
#include <SDL_events.h>
#include <SDL_video.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <numeric>
#include <optional>
#include <set>
#include <vulkan/vulkan_core.h>

void RendererContext::drawFrame() {
  // wait draw command queue for the frame
  vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE,
                  UINT64_MAX);
  // wait for transfer queue to finish
  VkSemaphoreWaitInfo waitInfo;
  waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
  waitInfo.pNext = NULL;
  // If VK_SEMAPHORE_WAIT_ANY_BIT is not set, the semaphore wait condition
  // is that all of the semaphores in VkSemaphoreWaitInfo::pSemaphores have
  // reached the value specified by the corresponding element of
  // VkSemaphoreWaitInfo::pValues.
  waitInfo.flags = 0;
  auto &frameSemaphores = transferSemaphores[currentFrame];
  waitInfo.pSemaphores = frameSemaphores.data();
  waitInfo.semaphoreCount = frameSemaphores.size();
  vkWaitSemaphores(device, &waitInfo, UINT64_MAX);
  while (windowMinimizedOrHidden || (scr_width == 0 || scr_height == 0)) {
    SDL_WaitEvent(NULL);
  }
  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(
      device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame],
      VK_NULL_HANDLE, &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  // Only reset the fence if we are submitting work
  vkResetFences(device, 1, &inFlightFences[currentFrame]);

  recordCommandBuffer(drawCommandBuffer, imageIndex);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers =
      &dynamic_draw_commands.commandBuffers[currentFrame];

  VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  if (vkQueueSubmit(graphicsQueue, 1, &submitInfo,
                    inFlightFences[currentFrame]) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {swapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;

  presentInfo.pImageIndices = &imageIndex;

  presentInfo.pResults = nullptr; // Optional

  result = vkQueuePresentKHR(presentQueue, &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      framebufferResized) {
    framebufferResized = false;
    recreateSwapChain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void RendererContext::init() {
  SDL_Init(SDL_INIT_VIDEO);
  recreateWindow();

  createInstance();
  createSurface();
  pickPhysicalDevice();
  queueFamilyIndices = findQueueFamilies(physicalDevice);
  createLogicalDevice();
  createSwapChain();
  createImageViews();
  createRenderPass();
  createGraphicsPipeline();
  createFramebuffers();
  drawCommandPool =
      createCommandPoolTransient(queueFamilyIndices.graphicsFamily->id);
  createVertexBuffer();
  createCommandBuffer();
  createSyncObjects();
}
void RendererContext::recreateWindow() {
  SDL_WindowFlags window_flags =
      (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE |
                        SDL_WINDOW_ALLOW_HIGHDPI // SDL_WINDOW_OPENGL
      );

  window = SDL_CreateWindow("voxel_world", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, scr_width, scr_height,
                            window_flags);
  if (!window)
    LOG_ERROR("Couldn't create window");
}
int RendererContext::windowCallback(SDL_Event *e) {
  if (e->window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
      e->window.event == SDL_WINDOWEVENT_RESIZED) {
    resizeWindow(e);
  } else if (e->window.event == SDL_WINDOWEVENT_MINIMIZED) {
    minimizedWindow();
  } else if (e->window.event == SDL_WINDOWEVENT_HIDDEN) {
    hiddenWindow();
  } else if (e->window.event == SDL_WINDOWEVENT_RESTORED) {
    restoredWindow();
  }

  return 0; // Value will be ignored
}
void RendererContext::resizeWindow(SDL_Event *e) {
  if (scr_width != e->window.data1 || scr_height != e->window.data2) {
    scr_width = e->window.data1;
    scr_height = e->window.data2;
    SDL_SetWindowSize(window, scr_width, scr_height);
    framebufferResized = true;
  }
}
void RendererContext::minimizedWindow() { windowMinimizedOrHidden = true; }

void RendererContext::hiddenWindow() { windowMinimizedOrHidden = true; }

void RendererContext::restoredWindow() { windowMinimizedOrHidden = false; }

void RendererContext::recreateSwapChain() {
  while (windowMinimizedOrHidden || (scr_width == 0 || scr_height == 0)) {
    SDL_WaitEvent(NULL);
  }
  vkDeviceWaitIdle(device);

  cleanupSwapChain();

  createSwapChain();
  createImageViews();
  createFramebuffers();
}
void RendererContext::clear() {
  vkDeviceWaitIdle(device);
  cleanupSwapChain();

  vkDestroyPipeline(device, graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  vkDestroyRenderPass(device, renderPass, nullptr);

  vkDestroyBuffer(device, deviceBuffer, nullptr);
  vkFreeMemory(device, deviceBufferMemory, nullptr);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
    vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
    vkDestroyFence(device, inFlightFences[i], nullptr);
  }

  vkDestroyCommandPool(device, drawCommandPool, nullptr);
  {
    for (auto &transfer_commands : transferWorkers) {
      vkDestroyCommandPool(device, transfer_commands.commandPool, NULL);
    }
    transferWorkers.clear();
  }

  vkDestroyDevice(device, nullptr);

  if (enableValidationLayers) {
    DestroyDebugUtilsMessengerEXT(instance, nullptr);
  }

  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);

  SDL_DestroyWindow(window);
  SDL_Quit();
}

VKAPI_ATTR VkBool32 VKAPI_CALL RendererContext::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData) {

  const char *severity;
  const char *type;
  switch (messageSeverity) {
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
    severity = "VERBOSE";
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    severity = "INFO";
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
    severity = "WARNING";
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
    severity = "ERROR";
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
    severity = "MAX";
    break;
  }
  switch (messageType) {
  case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
    type = "GENERAL";
  case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
    type = "VALIDATION";
  case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
    type = "PERFORMANCE";
  case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT:
    type = "DEVICE_ADDRESS_BINDING";
  case VK_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BITS_MAX_ENUM_EXT:
    type = "FLAG_BITS_MAX_ENUM";
  }
  std::cerr << severity << ": "
            << " Vulkan type: " << type
            << " Message: " << pCallbackData->pMessage << std::endl;
  return VK_FALSE;
}
VkResult RendererContext::CreateDebugUtilsMessengerEXT(
    const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void RendererContext::DestroyDebugUtilsMessengerEXT(
    VkInstance instance, const VkAllocationCallbacks *pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

void RendererContext::setupDebugCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT &debugCreateInfo) {
  debugCreateInfo.sType =
      VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  debugCreateInfo.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  debugCreateInfo.pfnUserCallback = debugCallback;
  debugCreateInfo.pUserData = nullptr; // Optional
};
QueueFamilyIndices RendererContext::findQueueFamilies(
    VkPhysicalDevice &physical_device_candidate) {
  QueueFamilyIndices indices;
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device_candidate,
                                           &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(
      physical_device_candidate, &queueFamilyCount, queueFamilies.data());
  // Assign index to queue families that could be found
  uint32_t i = 0;
  std::vector<QueueFamily> graphicsFamilies;
  std::vector<QueueFamily> presentFamilies;
  std::vector<QueueFamily> transferFamilies;
  std::vector<QueueFamily> computeFamilies;

  for (const auto &queueFamily : queueFamilies) {
    auto is_graphics = queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT;
    if (is_graphics) {
      graphicsFamilies.push_back(
          {i, 0, queueFamily.queueCount, queueFamily.queueFlags});
    }
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(physical_device_candidate, i, surface,
                                         &presentSupport);
    if (presentSupport) {
      presentFamilies.push_back(
          {i, 0, queueFamily.queueCount, queueFamily.queueFlags});
    }
    // seek dedicated compute and transfer. compute also support transfer, but
    // transfer specialised is better since its DMA.
    if (!is_graphics) {
      if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
        computeFamilies.push_back(
            {i, 0, queueFamily.queueCount, queueFamily.queueFlags});
      } else if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
        transferFamilies.push_back(
            {i, 0, queueFamily.queueCount, queueFamily.queueFlags});
      }
    }
    i++;
  }
  if (graphicsFamilies.empty()) {
    throw std::runtime_error("not found graphicsFamily");
  }
  if (presentFamilies.empty()) {
    throw std::runtime_error("not found presentFamily");
  }
  // determinte graphics/present queues
  // prioritise same queue, to not do thransfer step
  // My initial unexperienced assumption. to be revised. probably can benefit
  // from separate graphics/present
  std::optional<QueueFamily> graphicsFamily, presentFamily;
  for (auto i : graphicsFamilies) {
    if (std::find_if(presentFamilies.begin(), presentFamilies.end(),
                     [&i](QueueFamily &f) { return f.id == i.id; }) !=
        presentFamilies.end()) {
      graphicsFamily = i;
      presentFamily = i;
      break;
    }
  }
  // need only 1 queue for graphics
  if (!graphicsFamily.has_value()) {
    graphicsFamily = graphicsFamilies.at(0);
    graphicsFamily->num_queues = 1;
    graphicsFamilies.at(0).advance_to_next_queue();
  }

  if (!presentFamily.has_value()) {
    presentFamily = presentFamilies.at(0);
    presentFamily->num_queues = 1;
    presentFamilies.at(0).advance_to_next_queue();
  }
  indices.presentFamily = presentFamily;
  indices.graphicsFamily = graphicsFamily;
  if (!computeFamilies.empty()) {
    indices.computeFamily = computeFamilies.at(0);
    computeFamilies.erase(computeFamilies.begin());
  }
  // DEFINE TRANSFER QUEUES
  // //////////////////////////////////////////////////////////
  setTransferFamily(transferFamilies, graphicsFamilies, computeFamilies,
                    indices);
  // DEFINE TRANSFER QUEUES END
  // //////////////////////////////////////////////////
  return indices;
}

void RendererContext::setTransferFamily(
    std::vector<QueueFamily> &transferFamilies,
    std::vector<QueueFamily> &graphicsFamilies,
    std::vector<QueueFamily> &computeFamilies, QueueFamilyIndices &indices) {
  int transfer_family_index = 0;
  auto transfer_iter = transferFamilies.begin();
  auto graphics_iter = graphicsFamilies.begin();
  auto compute_iter = computeFamilies.begin();
  bool has_transfer = false;
  bool has_graphics = false;
  bool has_compute = false;
  bool init = true;
  // const int NUM_TRANSFER_QUEUES = 2;
  const int NUM_TRANSFER_QUEUES = 1;
  std::array<QueueFamily, NUM_TRANSFER_QUEUES> transferFamily;
  // try find DNA special transfer queue. with fallback.
  while (((has_transfer = (transfer_iter != transferFamilies.end() &&
                           transfer_iter->has_available())) ||
          (has_graphics = (graphics_iter != graphicsFamilies.end() &&
                           graphics_iter->has_available())) ||
          (has_compute = (compute_iter != computeFamilies.end() &&
                          compute_iter->has_available()))) &&
         transfer_family_index < NUM_TRANSFER_QUEUES) {
    if (has_transfer) {
      if (transfer_iter->num_queues <= NUM_TRANSFER_QUEUES) {
        transferFamily = {*transfer_iter};
        break;
      }
    }
    QueueFamily &out = transferFamily.at(transfer_family_index);
    QueueFamily next_c;

    std::array<QueueFamily, NUM_TRANSFER_QUEUES> transferFamily;

    if (has_transfer) {
      next_c = *transfer_iter;
      transfer_iter->advance_to_next_queue();
    } else if (has_graphics) {
      next_c = *graphics_iter;
      graphics_iter->advance_to_next_queue();
    } else if (has_compute) {
      next_c = *compute_iter;
      compute_iter->advance_to_next_queue();
    }
    if (init) {
      out = next_c;
      out.num_queues = 1;
    } else if (transferFamily[transfer_family_index].id == next_c.id) {
      out.num_queues++;
    } else {
      transfer_family_index++;
      if (transfer_family_index >= NUM_TRANSFER_QUEUES) {
        break;
      }
      QueueFamily &out2 = transferFamily.at(transfer_family_index);
      out2 = next_c;
      out2.num_queues = 1;
    }
    uint32_t num_queues_res = 0;
    for (auto &f : transferFamily) {
      num_queues_res += f.num_queues;
    }
    if (num_queues_res >= NUM_TRANSFER_QUEUES) {
      break;
    }
    init = false;
  }
  // refactored to have a single transfer queue
  indices.transferFamily = {*transferFamily.begin()};
}

bool RendererContext::isDeviceSuitable(VkPhysicalDevice &device) {
  VkPhysicalDeviceProperties deviceProperties;
  VkPhysicalDeviceFeatures deviceFeatures;
  vkGetPhysicalDeviceProperties(device, &deviceProperties);
  vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

  return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
         deviceFeatures.geometryShader;

  QueueFamilyIndices indices = findQueueFamilies(device);
  bool extensionsSupported = checkDeviceExtensionSupport(device);
  bool swapChainAdequate = false;
  if (extensionsSupported) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
    swapChainAdequate = !swapChainSupport.formats.empty() &&
                        !swapChainSupport.presentModes.empty();
  }
  return indices.isCompleteGraphics() && extensionsSupported &&
         swapChainAdequate;
}

int RendererContext::rateDeviceSuitability(
    VkPhysicalDeviceProperties &deviceProperties,
    VkPhysicalDeviceFeatures &deviceFeatures) {

  int score = 0;

  // Discrete GPUs have a significant performance advantage
  if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
    score += 1000;
  }

  // Maximum possible size of textures affects graphics quality
  score += deviceProperties.limits.maxImageDimension2D;

  // Application can't function without geometry shaders
  if (!deviceFeatures.geometryShader) {
    return 0;
  }
  return score;
}

void RendererContext::pickPhysicalDevice() {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
  if (deviceCount == 0) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }
  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
  // Use an ordered map to automatically sort candidates by increasing score
  std::multimap<int, VkPhysicalDevice> candidates;

  for (auto physical_device_candidate : devices) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physical_device_candidate, &deviceProperties);
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(physical_device_candidate, &deviceFeatures);
    if (isDeviceSuitable(physical_device_candidate)) {
      int score = rateDeviceSuitability(deviceProperties, deviceFeatures);
      candidates.insert(std::make_pair(score, physical_device_candidate));
    }
  }
  this->physicalDevice = candidates.begin()->second;
}

void RendererContext::createLogicalDevice() {
  VkPhysicalDeviceFeatures deviceFeatures{};

  // QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
  QueueFamilyIndices &indices = queueFamilyIndices;
  std::vector<VkDeviceQueueCreateInfo> queues =
      getPhysicalDeviceQueues(indices);
  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pQueueCreateInfos = queues.data();
  createInfo.queueCreateInfoCount = queues.size();
  createInfo.pEnabledFeatures = &deviceFeatures;
  createInfo.enabledExtensionCount = deviceExtensions.size();
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();

  if (enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
  } else {
    createInfo.enabledLayerCount = 0;
  }
  if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create logical device!");
  }
  vkGetDeviceQueue(device, indices.graphicsFamily.value().id,
                   indices.graphicsFamily.value().start_queue_num,
                   &graphicsQueue);
  vkGetDeviceQueue(device, indices.presentFamily.value().id,
                   indices.presentFamily.value().start_queue_num,
                   &presentQueue);
  vkGetDeviceQueue(device, indices.transferFamily.value().id,
                   indices.transferFamily.value().start_queue_num,
                   &transferQueue);
}

void RendererContext::createInstance() {
  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Hello Triangle";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "pEngineName";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;
  uint32_t extensions_count;
  ////////////////////////////////////////////////////////////////////////////////
  bool res1 = SDL_Vulkan_GetInstanceExtensions(window, &extensions_count, 0);
  if (!res1) {
    throw std::runtime_error(
        "Unexpected false returned from initial extension count request to "
        "SDL_Vulkan_GetInstanceExtensions");
  }
  std::vector<const char *> extensions(extensions_count);
  res1 = SDL_Vulkan_GetInstanceExtensions(window, &extensions_count,
                                          extensions.data());
  auto i1 = extensions.begin();
  // it for any reason creates 3 array with nullptr as last
  while (&i1 != nullptr && i1 != extensions.end()) {
    if (*i1 == nullptr) {
      i1 = extensions.erase(i1);
    } else {
      i1++;
    }
  }
  if (enableValidationLayers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }
  if (!res1) {
    throw std::runtime_error(
        "Unexpected false returned from initial extension count request to "
        "SDL_Vulkan_GetInstanceExtensions");
  }
  //////////////////////////////////////////////////////////////////////////////////
  VkInstanceCreateInfo createInfo{};
  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = extensions.size();
  createInfo.ppEnabledExtensionNames = extensions.data();
  if (enableValidationLayers) {
    if (!checkValidationLayerSupport()) {
      throw std::runtime_error(
          "validation layers requested, but not available!");
    }
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
    setupDebugCreateInfo(debugCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
  } else {
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;
  }
  VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
  if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to create instance!");
  }
}

bool RendererContext::checkValidationLayerSupport() {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const char *layerName : validationLayers) {
    bool layerFound = false;

    for (const auto &layerProperties : availableLayers) {
      if (strcmp(layerName, layerProperties.layerName) == 0) {
        layerFound = true;
        break;
      }
    }
    if (!layerFound) {
      return false;
    }
  }
  return true;
}
void RendererContext::createSurface() {
  if (SDL_Vulkan_CreateSurface(window, instance, &surface) != SDL_TRUE) {
    throw std::runtime_error("failed to create window surface!");
  }
}
std::vector<VkDeviceQueueCreateInfo>
RendererContext::getPhysicalDeviceQueues(QueueFamilyIndices &indices) {

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  float queuePriority = 1.0f;
  auto &graphics_queue = indices.graphicsFamily.value();
  auto &present_queue = indices.presentFamily.value();
  auto &transfer_queue = indices.transferFamily.value();

  // for (auto &q : indices.transferFamily) {
  //   if (q.id == graphics_queue.id) {
  //     num_transfer_from_graphics_family++;
  //   } else if (!is_graphics_present_same && q.id == present_queue.id) {
  //     num_transfer_from_present_family++;
  //   } else {
  //     VkDeviceQueueCreateInfo queueCreateInfo{};
  //     queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  //     queueCreateInfo.queueFamilyIndex = q.id;
  //     queueCreateInfo.queueCount = q.num_queues;
  //     queueCreateInfo.pQueuePriorities = &queuePriority;
  //     queueCreateInfos.push_back(queueCreateInfo);
  //   }
  // }

  VkDeviceQueueCreateInfo queueCreateInfo{};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = graphics_queue.id;
  queueCreateInfo.queueCount = 1;
  queueCreateInfo.pQueuePriorities = &queuePriority;
  queueCreateInfos.push_back(queueCreateInfo);

  if (graphics_queue.id != present_queue.id) {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = present_queue.id;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }
  if (graphics_queue.id != transfer_queue.id) {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = indices.transferFamily->id;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }
  return queueCreateInfos;
}
bool RendererContext::checkDeviceExtensionSupport(
    VkPhysicalDevice &physical_device_candidate) {
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(physical_device_candidate, nullptr,
                                       &extensionCount, nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(physical_device_candidate, nullptr,
                                       &extensionCount,
                                       availableExtensions.data());

  std::set<std::string> requiredExtensions(deviceExtensions.begin(),
                                           deviceExtensions.end());

  for (const auto &extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}
SwapChainSupportDetails
RendererContext::querySwapChainSupport(VkPhysicalDevice &physical_device) {
  SwapChainSupportDetails details;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface,
                                            &details.capabilities);
  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formatCount,
                                       nullptr);

  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formatCount,
                                         details.formats.data());
  }
  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface,
                                            &presentModeCount, nullptr);

  if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface,
                                              &presentModeCount,
                                              details.presentModes.data());
  }
  return details;
}
VkSurfaceFormatKHR RendererContext::chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats) {
  for (const auto &availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }

  return availableFormats[0];
}
VkPresentModeKHR RendererContext::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes) {
  for (const auto &availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return availablePresentMode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}
void RendererContext::createSwapChain() {
  SwapChainSupportDetails swapChainSupport =
      querySwapChainSupport(physicalDevice);

  VkSurfaceFormatKHR surfaceFormat =
      chooseSwapSurfaceFormat(swapChainSupport.formats);
  VkPresentModeKHR presentMode =
      chooseSwapPresentMode(swapChainSupport.presentModes);
  VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface;

  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
  uint32_t queueFamilyIndices[] = {indices.graphicsFamily->id,
                                   indices.presentFamily->id};

  if (indices.graphicsFamily->id != indices.presentFamily->id) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;     // Optional
    createInfo.pQueueFamilyIndices = nullptr; // Optional
  }
  createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create swap chain!");
  }

  vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
  swapChainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(device, swapChain, &imageCount,
                          swapChainImages.data());
  swapChainImageFormat = surfaceFormat.format;
  swapChainExtent = extent;
}

VkExtent2D RendererContext::chooseSwapExtent(
    const VkSurfaceCapabilitiesKHR &capabilities) {
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    int w, h;
    // This may differ from SDL_GetWindowSize() if we're rendering to a
    // high-DPI
    SDL_Vulkan_GetDrawableSize(window, &w, &h);

    VkExtent2D actualExtent = {static_cast<uint32_t>(w),
                               static_cast<uint32_t>(h)};
    actualExtent.width =
        std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    return actualExtent;
  }
}
void RendererContext::createImageViews() {
  swapChainImageViews.resize(swapChainImages.size());
  for (size_t i = 0; i < swapChainImages.size(); i++) {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = swapChainImages[i];
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = swapChainImageFormat;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device, &createInfo, nullptr,
                          &swapChainImageViews[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create image views!");
    }
  }
};

std::vector<char> RendererContext::readShaderFile(const char *filename) {
  std::filesystem::path p = XSTR(VULKAN_SPV_DIR);
  p /= filename;
  std::ifstream file(p, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("failed to open file!");
  }
  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();

  return buffer;
}

VkShaderModule
RendererContext::createShaderModule(const std::vector<char> &code) {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());
  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }
  return shaderModule;
}

void RendererContext::createGraphicsPipeline() {
  auto vertShaderCode = readShaderFile("shader_vert.spv");
  auto fragShaderCode = readShaderFile("shader_frag.spv");
  VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
  VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                    fragShaderStageInfo};
  auto bindingDescription = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; // Optional
  vertexInputInfo.vertexAttributeDescriptionCount =
      attributeDescriptions.size();
  vertexInputInfo.pVertexAttributeDescriptions =
      attributeDescriptions.data(); // Optional

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)swapChainExtent.width;
  viewport.height = (float)swapChainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swapChainExtent;

  /* When opting for dynamic viewport(s) and scissor rectangle(s)
   * you need to enable the respective dynamic states for the pipeline:
   */
  std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                               VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates = dynamicStates.data();

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;

  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f; // Optional
  rasterizer.depthBiasClamp = 0.0f;          // Optional
  rasterizer.depthBiasSlopeFactor = 0.0f;    // Optional

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f;          // Optional
  multisampling.pSampleMask = nullptr;            // Optional
  multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
  multisampling.alphaToOneEnable = VK_FALSE;      // Optional

  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  // blend disable, color replace
  {
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;             // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; //  Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;             // Optional
  }

  // blend base on alpha, transparency
  /*
  {
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor =
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
  }
  */

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  // Optional, set to VK_TRUE If you want to use the
  // second method of blending (bitwise combination) colorBlending.logicOpEnable
  // = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;

  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f; // Optional
  colorBlending.blendConstants[1] = 0.0f; // Optional
  colorBlending.blendConstants[2] = 0.0f; // Optional
  colorBlending.blendConstants[3] = 0.0f; // Optional

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;            // Optional
  pipelineLayoutInfo.pSetLayouts = nullptr;         // Optional
  pipelineLayoutInfo.pushConstantRangeCount = 0;    // Optional
  pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                             &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = nullptr; // Optional
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  // pipelineInfo.pDynamicState = nullptr;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;

  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
  pipelineInfo.basePipelineIndex = -1;              // Optional

  // TODO: The big advantage of a pipeline cache is that the pipeline state can
  // be saved to a file to be used between runs of an application, eliminating
  // some of the costly parts of creation. There is a great Khronos presentation
  // on pipeline caching from SIGGRAPH 2016
  // https://docs.vulkan.org/guide/latest/pipeline_cache.html
  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &graphicsPipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to create graphics pipeline!");
  }

  vkDestroyShaderModule(device, fragShaderModule, nullptr);
  vkDestroyShaderModule(device, vertShaderModule, nullptr);
};
void RendererContext::createRenderPass() {
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = swapChainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create render pass!");
  }
}
void RendererContext::createFramebuffers() {
  swapChainFramebuffers.resize(swapChainImageViews.size());
  for (size_t i = 0; i < swapChainImageViews.size(); i++) {
    VkImageView attachments[] = {swapChainImageViews[i]};

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = swapChainExtent.width;
    framebufferInfo.height = swapChainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
                            &swapChainFramebuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
};
VkCommandPool
RendererContext::createCommandPool(VkCommandPoolCreateFlagBits &flags,
                                   uint32_t queueFamilyIndex) {
  VkCommandPool commandPool;
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = flags;
  poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily->id;
  if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
  }
  return commandPool;
}
/**
 * Use for transfer worksers
 */
VkCommandPool
RendererContext::createCommandPoolTransient(uint32_t queueFamilyIndex) {
  // Both flags - short-lived AND need individual reset capability
  // auto flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
  //              VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  // If you reset the whole pool per frame instead
  auto flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  createCommandPool(flags, queueFamilyIndex);
}
/**
 * Use for static geometry
 */
VkCommandPool
RendererContext::createCommandPoolPersistent(uint32_t queueFamilyIndex) {
  auto flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  // No TRANSIENT_BIT - these command buffers live for many frames
  createCommandPool(flags, queueFamilyIndex);
}
std::vector<VkCommandBuffer>
RendererContext::createCommandBuffer(VkCommandPool commandPool,
                                     int numBuffers) {
  std::vector<VkCommandBuffer> commandBuffers;
  commandBuffers.resize(numBuffers);
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = numBuffers;

  if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }
  return commandBuffers;
}
void RendererContext::recordCommandBufferDraw(VkCommandBuffer commandBuffer,
                                              uint32_t imageIndex) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0;                  // Optional
  beginInfo.pInheritanceInfo = nullptr; // Optional

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }
  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];

  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = swapChainExtent;

  VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    graphicsPipeline);
  VkBuffer vertexBuffers[] = {deviceBuffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(swapChainExtent.width);
  viewport.height = static_cast<float>(swapChainExtent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swapChainExtent;
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);

  vkCmdEndRenderPass(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }
}
void RendererContext::createSyncObjects() {
  imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

  // Vulkan 1.2 timeline semaphore - host/gpu synchronization combined.
  // While some inconvenient limitations in the API remain, the timeline
  // semaphore programming model should greatly reduce the need for host-side
  // synchronization and the number of synchronization objects Vulkan
  // applications are required to track, thereby reducing host-side stalls and
  // application complexity, which should in turn increase performance and
  // quality. As such,
  // !!! the Vulkan working group highly encourages all developers to make the
  // switch to timeline semaphores for all coarse-grained
  // https://www.khronos.org/blog/vulkan-timeline-semaphores

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  VkSemaphoreTypeCreateInfo timelineCreateInfo;
  timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
  timelineCreateInfo.pNext = NULL;
  timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
  timelineCreateInfo.initialValue = 0;

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  semaphoreInfo.pNext = &timelineCreateInfo;
  semaphoreInfo.flags = 0;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) !=
            VK_SUCCESS) {

      throw std::runtime_error(
          "failed to create synchronization objects for a frame!");
    }
  }
}
void RendererContext::cleanupSwapChain() {
  // delete commented. just tried to get rid off vkDestroyFramebuffer
  // complaining pending command buffer when exiting program. no luck yet.
  // vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE,
  //                 UINT64_MAX);
  // vkFreeCommandBuffers(device, commandPool, commandBuffers.size(),
  //                      commandBuffers.data());
  for (auto framebuffer : swapChainFramebuffers) {
    vkDestroyFramebuffer(device, framebuffer, nullptr);
  }
  for (auto imageView : swapChainImageViews) {
    vkDestroyImageView(device, imageView, nullptr);
  }
  vkDestroySwapchainKHR(device, swapChain, nullptr);
}
void RendererContext::createStagingBuffer(size_t bufferSize) {
  size_t bufferSize = sizeof(vertices[0]) * vertices.size();
  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuffer, stagingBufferMemory);
  {
    void *data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);
  }
}
// Keep mapped for the entire lifetime of the buffer
// Unmap only when destroying the buffer
void RendererContext::destroyStagingBuffer(StagingBuffer &stagingBuffer) {
  vkUnmapMemory(device, staging.memory); // Only unmap here
  vkDestroyBuffer(device, staging.buffer, nullptr);
  vkFreeMemory(device, staging.memory, nullptr);
}
void RendererContext::createVertexBuffer() {
  createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
          VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, deviceBuffer, deviceBufferMemory);
  copyBuffer(stagingBuffer, deviceBuffer, bufferSize);

  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);
}
uint32_t RendererContext::findMemoryType(uint32_t typeFilter,
                                         VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}
void RendererContext::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                   VkMemoryPropertyFlags properties,
                                   VkBuffer &buffer,
                                   VkDeviceMemory &bufferMemory) {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to create buffer!");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      findMemoryType(memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to allocate buffer memory!");
  }

  vkBindBufferMemory(device, buffer, bufferMemory, 0);
}
void RendererContext::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                                 VkDeviceSize size) {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0; // Optional
  copyRegion.dstOffset = 0; // Optional
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
  vkEndCommandBuffer(commandBuffer);

  // This command buffer only contains the copy command, so we can stop
  // recording right after that. Now execute the command buffer to complete
  // the transfer:
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  // either fence to not block or WaitIdle to block. TODO: dont block.
  // wait prev fence at beginning before transfering latest vertex data? or
  // `reset` command to not fallback? but staging buffer is always up to date.
  // we have to block when doing staging to device buf trans, have to block
  // all threads that do staging write? or we have staging per thread yep, so
  // only block thread that does transfer. so current solution is ok, just
  // create transient queue/comBuf for transfer
  vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphicsQueue);
  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}
