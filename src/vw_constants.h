#ifndef VW_CONSTANTS_H
#define VW_CONSTANTS_H


// SausageV2 legacy. TODO: refactor to single Vulkan buffer with offsets.
// need to calculate total size based on estimated number of vertices, indices, etc.
namespace BufferSettings_SausageV2_Legacy {
constexpr unsigned long MAX_VERTEX = 1000000;
constexpr unsigned long MAX_INDEX = 2 * MAX_VERTEX;
constexpr unsigned long MAX_BASE_MESHES = 400;
constexpr unsigned long MAX_MESHES_INSTANCES = MAX_BASE_MESHES * 10;
constexpr unsigned long MAX_BONES = 100;

constexpr unsigned long MAX_VERTEX_STATIC = 1000000;
constexpr unsigned long MAX_INDEX_STATIC = 2 * MAX_VERTEX_STATIC;
constexpr unsigned long MAX_BASE_MESHES_STATIC = 40;
constexpr unsigned long MAX_MESHES_STATIC_INSTANCES =
    MAX_BASE_MESHES_STATIC * 10;

constexpr unsigned long MAX_MESHES_TERRAIN = 40;

constexpr unsigned long MAX_VERTEX_UI = 10000;
constexpr unsigned long MAX_INDEX_UI = 2 * MAX_VERTEX_UI;

constexpr unsigned long MAX_TEXTURE = 100;
constexpr unsigned int MAX_BLEND_TEXTURES = 4;
constexpr unsigned long MAX_LIGHTS = 100; // TODO: figure out correct size

constexpr unsigned long MAX_VERTEX_OUTLINE = 1500000;
constexpr unsigned long MAX_INDEX_OUTLINE = 2 * MAX_VERTEX_OUTLINE;

// max num of BlendTexture's for terrain chunk's tiles. (number of differently
// coloured tiles in chunk)
constexpr unsigned long TERRAIN_MAX_TEXTURES = 1000;
}; // namespace BufferSettings

#endif // !DEBUG
