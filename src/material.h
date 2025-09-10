#ifndef MATERIAL_H
#define MATERIAL_H
#include <cstdint>
#include <vulkan/vulkan.h>

// Structure for each material's indirect data bound to shader to determine
// offsets
struct Material {
  uint32_t materialID;
  uint32_t maxObjects;     // Max objects with this material
  uint32_t indirectOffset; // Offset in indirect buffer
  uint32_t countOffset;    // Offset in count buffer
};
// Structure for each material's indirect data
struct MaterialBatch {
  uint32_t materialID;
  uint32_t maxObjects;     // Max objects with this material
  uint32_t indirectOffset; // Offset in indirect buffer
  uint32_t countOffset;    // Offset in count buffer
  VkDescriptorSet descriptorSet;
};
// example
// MaterialBatch materials[] = {
//     {0, 5000, 0, 0, metalDescriptorSet},      // Metal: commands 0-4999
//     {1, 3000, 5000, 4, plasticDescriptorSet}, // Plastic: commands 5000-7999
//     {2, 2000, 8000, 8, glassDescriptorSet}    // Glass: commands 8000-9999
// };
#endif
