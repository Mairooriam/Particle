#include <vulkan/vulkan_core.h>

typedef enum VertexLayout {
  VERTEX_LAYOUT_POSITION = 0x0,
  VERTEX_LAYOUT_NORMAL = 0x1,
  VERTEX_LAYOUT_COLOR = 0x2,
  VERTEX_LAYOUT_UV = 0x3,
  VERTEX_LAYOUT_TANGENT = 0x4,
  VERTEX_LAYOUT_BITANGENT = 0x5,
  VERTEX_LAYOUT_DUMMY_FLOAT = 0x6,
  VERTEX_LAYOUT_DUMMY_VEC4 = 0x7
} VertexLayout;

struct MeshBufferInfo {
  VkBuffer buf;
  VkDeviceMemory mem;
  size_t size;
};

struct MeshBuffer {
  MeshBufferInfo vertices;
  MeshBufferInfo indices;
  uint32_t indexCount;
  glm::vec3 dim;
};
