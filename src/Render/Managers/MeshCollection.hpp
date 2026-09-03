#ifndef MESH_COLLECTION_HPP
#define MESH_COLLECTION_HPP

#include <cstdint>
#include <glm/glm.hpp>
#include <optional>
#include <span>
#include <vector>

// clang-format off
#include "../RHI/Vulkan/VulkanConfig.hpp"
// clang-format on

#include "BufferManager/IBufferManager.hpp"

namespace SE
{

constexpr size_t DEFAULT_RESERVED_DRAW_COMMANDS    = 1000;
constexpr size_t DEFAULT_RESERVED_DATA_COMMANDS    = 1000;
constexpr size_t DEFAULT_RESERVED_TRANSFORM_MATRIX = 1000;

using SeMeshCollectionID = uint32_t;
using SeMeshID           = uint32_t;
using SeSortKey          = uint64_t;

struct DrawIndexedIndirectCommand
{
    uint32_t indexCount    = 0;
    uint32_t instanceCount = 1;
    uint32_t firstIndex    = 0;
    int32_t  vertexOffset  = 0;
    uint32_t firstInstance = 0;
};

struct alignas(16) Transform
{
    // clang-format off
    glm::vec4 position { 0.0f, 0.0f, 0.0f, 1.0f };
    glm::vec4 rotation { 0.0f, 0.0f, 0.0f, 1.0f };
    glm::vec4 scale    { 1.0f, 1.0f, 1.0f, 0.0f };
    // clang-format on
};

struct MeshInstanceData
{
    uint32_t transformIndex;
};

struct SubMesh
{
    DrawIndexedIndirectCommand cmd;
    SeSortKey                  sortKey           = 0;  // TODO: need method for sorting
    uint32_t                   vertexOffsetBytes = 0;
    uint32_t                   indexOffsetBytes  = 0;
    bool                       isActive          = true;

    /* SSBO's */
    MeshInstanceData meshData;
};

enum class WhichType : uint32_t
{
    TRANSFORMS,
    STORAGE
};

class MeshCollection
{
   public:
    MeshCollection(IBufferManager* bufferMgr, SharedVulkanConfig* config, size_t maxVertexBytes, size_t maxIndexBytes);

    SeMeshID addMesh(std::span<const uint8_t> vertexData, std::span<const uint8_t> indexData, uint32_t indexCount, uint32_t vertexStride, SeSortKey sortKey);

    SeResult updateMeshData(WhichType update, SeMeshID meshId, std::span<const uint8_t> data);
    void     flushAsyncUploads();
    SeResult removeMesh(SeMeshID meshId);

    void buildDrawCommands();

    SeBufferID getVertexBuffer() const
    {
        return m_vertexBuffer;
    }

    SeBufferHandle getVertexBufferHandle() const
    {
        return m_bufferManager->getBufferHandle(m_vertexBuffer);
    }

    SeBufferID getIndexBuffer() const
    {
        return m_indexBuffer;
    }

    SeBufferID getStorageBuffer() const
    {
        return m_storageBuffer[*m_config->currentFrame];
    }

    SeBufferID getTransformBuffer() const
    {
        return m_transformsBuffer[*m_config->currentFrame];
    }

    SeBufferHandle getIndexBufferHandle() const
    {
        return m_bufferManager->getBufferHandle(m_indexBuffer);
    }

    SeBufferHandle getIndirectBufferhandle() const
    {
        return m_bufferManager->getBufferHandle(m_indirectBuffer);
    }

    SeBufferHandle getStorageBufferHandle() const
    {
        return m_bufferManager->getBufferHandle(m_storageBuffer[*m_config->currentFrame]);
    }

    SeBufferHandle getTransformsBufferHandle() const
    {
        return m_bufferManager->getBufferHandle(m_transformsBuffer[*m_config->currentFrame]);
    }

    uint32_t getIndirectCommandCount() const
    {
        return static_cast<uint32_t>(m_subMeshes.size());
    }

    std::optional<SubMesh>      getSubMesh(uint32_t meshIndex) const;
    const std::vector<SubMesh>& getAllSubMeshes() const
    {
        return m_subMeshes;
    }

    const uint32_t& getAvtiveMeshes()
    {
        return m_activeCommandCount;
    }

   private:
    SharedVulkanConfig* m_config = nullptr;

    IBufferManager* m_bufferManager = nullptr;

    SeBufferID m_vertexBuffer = SE_INVALID_BUFFER_ID;
    SeBufferID m_indexBuffer  = SE_INVALID_BUFFER_ID;

    std::vector<SeBufferID> m_storageBuffer;
    std::vector<SeBufferID> m_transformsBuffer;

    SeBufferID m_indirectBuffer = SE_INVALID_BUFFER_ID;

    uint32_t m_currentVertexOffset = 0;
    uint32_t m_currentIndexOffset  = 0;

    std::vector<SubMesh> m_subMeshes;
    uint32_t             m_activeCommandCount = 0;

    static std::span<const uint8_t> asBytes(const DrawIndexedIndirectCommand& cmd)
    {
        return {reinterpret_cast<const uint8_t*>(&cmd), sizeof(cmd)};
    }
};

}  // namespace SE

#endif /* MESH_COLLECTION_HPP */