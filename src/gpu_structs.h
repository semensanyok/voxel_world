#ifndef VW_GPU_STRUCTS_H
#define VW_GPU_STRUCTS_H

#include <array>
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>
#include "vw_constants.h"

using namespace glm;


// frustum culling data
struct ObjectData {
    mat4 modelMatrix;
    vec3 aabbMin;        // Local space AABB min
    vec3 aabbMax;        // Local space AABB max
    uint materialID;
    uint objectId; // index of base instance for the DrawCommand drawCommands[] array; 
};

// struct VertexStatic {
//   vec3 Position;
//   vec3 Normal;
//   vec2 TexCoords;
//   vec3 Tangent;
//   vec3 Bitangent;
// };
// struct Vertex {
//   vec3 Position;
//   vec3 Normal;
//   vec2 TexCoords;
//   vec3 Tangent;
//   vec3 Bitangent;
//   ivec4 BoneIds;
//   vec4 BoneWeights;
// };

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



namespace BufferSettings {
	constexpr unsigned long MAX_VERTEX = 1000000;
	constexpr unsigned long MAX_INDEX = 2 * MAX_VERTEX;
	// constexpr unsigned long MESH_DATA_SIZE = 16 * 1024 * 1024; // 16MB for static mesh
	constexpr unsigned long MAX_MESHES = 400;


	constexpr unsigned long STAGING_BUFFER_SIZE = 64 * 1024 * 1024; // 64MB per thread

	// TODO: 1 for metal 1 for plastic 1 for glass
	const int MAX_MATERIALS = 1;
	// frustum culling must have an atomic counter as a safeguard...
	// camera buffer + object buffer + indirect draw buffer + count buffer

	// BUFFER SIZES
constexpr unsigned long VERTEX_STORAGE_SIZE = MAX_VERTEX * sizeof(Vertex);
// constexpr unsigned long VERTEX_STATIC_STORAGE_SIZE =
//     MAX_VERTEX_STATIC * sizeof(VertexStatic);
// constexpr unsigned long VERTEX_UI_STORAGE_SIZE =
//     MAX_VERTEX_UI * sizeof(VertexUI);

constexpr unsigned long INDEX_STORAGE_SIZE = MAX_INDEX * sizeof(unsigned int);
// constexpr unsigned long INDEX_STATIC_STORAGE_SIZE =
//     MAX_INDEX_STATIC * sizeof(unsigned int);
// constexpr unsigned long INDEX_UI_STORAGE_SIZE =
//     MAX_INDEX_UI * sizeof(unsigned int);

const unsigned long DEVICE_BUFFER_SIZE = VERTEX_STORAGE_SIZE + INDEX_STORAGE_SIZE + MESH_DATA_SIZE;
const unsigned long DYNAMIC_BUFFER_SIZE = sizeof(CameraUBO) + MAX_MESHES * sizeof(ObjectData) + MAX_MATERIALS * (sizeof(VkDeviceSize) + sizeof(VkDrawIndexedIndirectCommand) + MAX_MATERIALS);

// constexpr unsigned long LIGHT_STORAGE_SIZE = MAX_LIGHTS * sizeof(Light);
///////////
// UNIFORMS
///////////
// constexpr unsigned long MESH_UNIFORMS_STORAGE_SIZE = sizeof(UniformDataMesh);
// constexpr unsigned long MESH_STATIC_UNIFORMS_STORAGE_SIZE =
//     sizeof(UniformDataMeshStatic);
// constexpr unsigned long MESH_TERRAIN_UNIFORMS_STORAGE_SIZE =
//     sizeof(UniformDataMeshTerrain);
// constexpr unsigned long TRANSFORM_OFFSET_STORAGE_SIZE =
//     MAX_MESHES_INSTANCES * sizeof(unsigned int);
// constexpr unsigned long TEXTURE_HANDLE_BY_TEXTURE_ID_STORAGE_SIZE =
//     MAX_TEXTURE * sizeof(GLuint64);

// constexpr unsigned long UNIFORM_OVERLAY_3D_STORAGE_SIZE =
//     sizeof(UniformDataOverlay3D);
// constexpr unsigned long UNIFORM_UI_STORAGE_SIZE = sizeof(UniformDataUI);
// constexpr unsigned long UNIFORM_CONTROLLER_SIZE = sizeof(ControllerUniformData);

// constexpr unsigned long VERTEX_OUTLINE_STORAGE_SIZE =
//     MAX_VERTEX_OUTLINE * sizeof(VertexOutline);
// constexpr unsigned long INDEX_OUTLINE_STORAGE_SIZE =
//     MAX_INDEX_OUTLINE * sizeof(unsigned int);
}; // namespace BufferSizes


// stores:
// - for buffer - capacity, next allocation offset 
// - for mesh - offsets and allocations
struct BufferOffsets {
	VkDeviceSize vertexOffset;
	VkDeviceSize indexOffset;
	VkDeviceSize staticDataOffset; // After indices

	VkDeviceSize vertexCount;
	VkDeviceSize indexCount;
	VkDeviceSize staticDataCount; // in bytes

	
};
typedef struct {
		// Static geometry (single device-local allocation)
  	VkBuffer deviceBuffer;
  	VkDeviceMemory deviceBufferMemory;
  	BufferOffsets deviceBufferOffsets;
		// single buffer with offsets
		VkBuffer hostVisibleBuffer; // camera + object + indirect + count
  	VkDeviceMemory hostVisibleBufferMemory;

		void* hostVisibleData; // Keep mapped for the entire lifetime of the buffer

		// Dynamic data (separate host-visible allocations)
		// VkBuffer cameraBuffer;         // Small, updated every frame.
		// VkBuffer objectBuffer;         // Medium, updated frequently
		// VkBuffer indirectBuffer;       // Small, written by GPU
		// VkBuffer countBuffer;          // Tiny, written by GPU [MAX_MATERIALS]
} BufferLayout;

// Structure for each material's indirect data
struct MaterialBatch {
	uint32_t materialID;
	uint32_t maxObjects;     // Max objects with this material
	uint32_t indirectOffset; // Offset in indirect buffer
	uint32_t countOffset;    // Offset in count buffer, index to countBuffer[MAX_MATERIALS]
	VkDescriptorSet descriptorSet;
};
// Camera updates every frame - keep it host-visible
struct CameraUBO {
		mat4 viewProj;
		vec4 frustumPlanes[6];
		vec3 cameraPos;
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
