#ifndef MESH_COLLECTION_HPP
#define MESH_COLLECTION_HPP

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "BufferManager/IBufferManager.hpp"

namespace SE
{

constexpr size_t DEFAULT_RESERVED_DRAW_COMMANDS = 1000;

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

struct SubMesh
{
    DrawIndexedIndirectCommand cmd;
    SeSortKey                  sortKey           = 0;  // TODO: need method for sorting
    uint32_t                   vertexOffsetBytes = 0;
    uint32_t                   indexOffsetBytes  = 0;
    bool                       isActive          = true;
};

class MeshCollection
{
   public:
    MeshCollection(IBufferManager* bufferMgr, size_t maxVertexBytes, size_t maxIndexBytes);

    SeMeshID addMesh(std::span<const uint8_t> vertexData, std::span<const uint8_t> indexData, uint32_t indexCount, uint32_t vertexStride, SeSortKey sortKey);
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

    SeBufferHandle getIndexBufferHandle() const
    {
        return m_bufferManager->getBufferHandle(m_indexBuffer);
    }

    SeBufferHandle getIndirectBufferhandle() const
    {
        return m_bufferManager->getBufferHandle(m_indirectBuffer);
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
    IBufferManager* m_bufferManager = nullptr;

    SeBufferID m_vertexBuffer   = SE_INVALID_BUFFER_ID;
    SeBufferID m_indexBuffer    = SE_INVALID_BUFFER_ID;
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