#ifndef SW_RENDERER_CONTEXT
#define SW_RENDERER_CONTEXT

#include <SDL.h>
#include <SDL_stdinc.h>
#include <SDL_vulkan.h>
#include <cstdint>
#include <vulkan/vulkan_core.h>

struct ObjectData {
    mat4 modelMatrix;
    vec3 aabbMin;        // Local space AABB min
    vec3 aabbMax;        // Local space AABB max
    uint materialID;
    uint objectId; // index of base instance for the DrawCommand drawCommands[] array; 
};
// Create buffers
VkBuffer indirectBuffer, countBuffer, objectDataBuffer;
VkDeviceMemory indirectMemory, countMemory, objectMemory;

// for multiple materials - create more, for each  `struct Material`
// Indirect draw commands buffer
VkBufferCreateInfo indirectBufferInfo = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = MAX_OBJECTS * sizeof(VkDrawIndexedIndirectCommand),
    .usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, // For compute shader access
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE};

// Count buffer (single uint32_t)
VkBufferCreateInfo countBufferInfo = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = sizeof(uint32_t),
    .usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE};

// Object data buffer
VkBufferCreateInfo objectBufferInfo = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = MAX_OBJECTS * sizeof(ObjectData),
    .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE};

#endif
