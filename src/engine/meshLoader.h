#include <vulkan/vulkan_core.h>
#include <vma/vk_mem_alloc.h>

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

typedef struct {
  VkBuffer buf;
  VmaAllocation allocation; // in order to free it later
  VmaAllocationInfo
      allocationInfo; // allocationInfo.pMappedData = CPU-mapped pointer
  VkDeviceSize size;
} MeshBufferInfo;

typedef struct {
  // Single GPU buffer: [vertices | indices]
  // vertices.buf   = the VkBuffer
  // vertices.size  = byte size of vertex data (= index buffer offset)
  // indices.size   = byte size of index data
  MeshBufferInfo vertices;
  VkDeviceSize indicesSize; // buf/allocation unused; size + indexCount only
  uint32_t indexCount;
} MeshBuffer;

// typedef struct {
//   MeshBuffer buf;
//   void *pToGpu;
// } Mesh;
