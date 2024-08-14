#include "renderer_context.h"
#include "logging.h"
#include "settings_global.h"
#include <iostream>
#include <map>

void RendererContext::init() {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_WindowFlags window_flags =
      (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE |
                        SDL_WINDOW_ALLOW_HIGHDPI // SDL_WINDOW_OPENGL
      );

  window = SDL_CreateWindow("voxel_world", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, GameSettings::SCR_WIDTH,
                            GameSettings::SCR_HEIGHT, window_flags);
  if (!window)
    LOG_ERROR("Couldn't create window");

  create_vulkan_instance();
  pick_physical_device();
}
void RendererContext::clear() {
  if (enableValidationLayers) {
    DestroyDebugUtilsMessengerEXT(instance, nullptr);
  }
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
  std::cerr << severity << ": " << " Vulkan type: " << type
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
  int i = 0;
  for (const auto &queueFamily : queueFamilies) {
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = i;
    }

    i++;
  }
  return indices;
}

bool RendererContext::isDeviceSuitable(VkPhysicalDevice &device) {
  VkPhysicalDeviceProperties deviceProperties;
  VkPhysicalDeviceFeatures deviceFeatures;
  vkGetPhysicalDeviceProperties(device, &deviceProperties);
  vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

  return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
         deviceFeatures.geometryShader;

  QueueFamilyIndices indices = findQueueFamilies(device);
  return indices.graphicsFamily.has_value();
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

void RendererContext::pick_physical_device() {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
  if (deviceCount == 0) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }
  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
  // Use an ordered map to automatically sort candidates by increasing score
  std::multimap<int, VkPhysicalDevice> candidates;

  for (auto device : devices) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    if (isDeviceSuitable(device)) {
      int score = rateDeviceSuitability(deviceProperties, deviceFeatures);
      candidates.insert(std::make_pair(score, device));
    }
  }
  this->physical_device = candidates.begin()->second;
}

void RendererContext::create_logical_device(VkPhysicalDevice device) {
  QueueFamilyIndices indices = findQueueFamilies(device);

  VkDeviceQueueCreateInfo queueCreateInfo{};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
  queueCreateInfo.queueCount = 1;

  float queuePriority = 1.0f;
  queueCreateInfo.pQueuePriorities = &queuePriority;
  VkPhysicalDeviceFeatures deviceFeatures{};
}

void RendererContext::create_vulkan_instance() {
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
