#ifndef VW_GPU_STRUCTS_H
#define VW_GPU_STRUCTS_H

#include <array>
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>

struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
  }
  static std::array<VkVertexInputAttributeDescription, 2>
  getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);
    return attributeDescriptions;
  }
};

// https://developer.nvidia.com/vulkan-memory-management
// recommends using same buffer with offsets for vert/ind/uniform
//   For Buffer memory we recommend making use of the offset mechanism the API
//   provides. Just like in OpenGL, Vulkan allows to binding a range of a
//   buffer. The benefit is that we avoid CPU memory costs for lots of tiny
//   buffers, as well as cache misses by using just the same buffer object and
//   varying the offset.
//
//   TODO:
//   pow2 slots to avoid defragmentation? (from prev SasV2 engine)
//   for dynamic meshes
//   for initial voxel terrain no need, will use fixed set for marching cubes.
struct BufferOffsets {
  unsigned int vertex_offset;
  unsigned int index_offset;
  // unsigned int uniform_offset;

  unsigned int vertex_count;
  unsigned int index_count;
};

struct MeshLoadData {
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
};

// Mesh metadata, offsets required for rendering
struct MeshData {
  BufferOffsets buffer_meta;
};

#endif
