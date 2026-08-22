#include "MeshCollection.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>

namespace SE
{

MeshCollection::MeshCollection(IBufferManager* bufferMgr, size_t maxVertexBytes, size_t maxIndexBytes) : m_bufferManager(bufferMgr)
{
    assert(m_bufferManager && "BufferManager cannot be null!");

    BufferDesc vDesc {};
    vDesc.size       = maxVertexBytes;
    vDesc.usage      = BufferUsage::VERTEX | BufferUsage::TRANSFER_DST;
    vDesc.memoryType = BufferMemoryType::DEVICE_LOCAL;
    m_vertexBuffer   = m_bufferManager->createBuffer(vDesc);

    BufferDesc iDesc {};
    iDesc.size       = maxIndexBytes;
    iDesc.usage      = BufferUsage::INDEX | BufferUsage::TRANSFER_DST;
    iDesc.memoryType = BufferMemoryType::DEVICE_LOCAL;
    m_indexBuffer    = m_bufferManager->createBuffer(iDesc);

    BufferDesc indirectDesc {};
    indirectDesc.size       = DEFAULT_RESERVED_DRAW_COMMANDS * sizeof(DrawIndexedIndirectCommand);
    indirectDesc.usage      = BufferUsage::INDIRECT | BufferUsage::TRANSFER_DST;
    indirectDesc.memoryType = BufferMemoryType::DEVICE_LOCAL;
    m_indirectBuffer        = m_bufferManager->createBuffer(indirectDesc);
}

SeMeshID MeshCollection::addMesh(std::span<const uint8_t> vertexData, std::span<const uint8_t> indexData, uint32_t indexCount, uint32_t vertexStride, SeSortKey sortKey)
{
    SubMesh sub {};
    sub.vertexOffsetBytes = m_currentVertexOffset;
    sub.indexOffsetBytes  = m_currentIndexOffset;
    sub.sortKey           = sortKey;
    sub.isActive          = true;

    sub.cmd.indexCount    = indexCount;
    sub.cmd.instanceCount = 1;
    sub.cmd.firstIndex    = m_currentIndexOffset / sizeof(uint32_t);
    sub.cmd.vertexOffset  = static_cast<int32_t>(m_currentVertexOffset / vertexStride);
    sub.cmd.firstInstance = 0;

    if (m_bufferManager->populateBuffer(m_vertexBuffer, m_currentVertexOffset, vertexData) != SE_SUCCESS
        || m_bufferManager->populateBuffer(m_indexBuffer, m_currentIndexOffset, indexData) != SE_SUCCESS)
        return SE_INVALID_MESH_ID;

    m_currentVertexOffset += static_cast<uint32_t>(vertexData.size());
    m_currentIndexOffset += static_cast<uint32_t>(indexData.size());

    const uint32_t commandIndex = static_cast<uint32_t>(m_subMeshes.size());

    m_subMeshes.push_back(sub);
    return commandIndex;
}

SeResult MeshCollection::removeMesh(SeMeshID meshId)
{
    if (meshId == SE_INVALID_MESH_ID || meshId >= m_subMeshes.size())
        return SE_FAILED_TO_DESTROY;

    m_subMeshes[meshId].isActive = false;

    return SE_SUCCESS;
}

void MeshCollection::buildDrawCommands()
{
    std::vector<const SubMesh*> active;
    active.reserve(m_subMeshes.size());

    for (const auto& sub : m_subMeshes)
        if (sub.isActive)
            active.push_back(&sub);

    std::sort(active.begin(), active.end(),
              [](const SubMesh* a, const SubMesh* b)
              {
                  return a->sortKey < b->sortKey;
              });
    // TODO: add new variable to check when can be new pipeline swap

    std::vector<DrawIndexedIndirectCommand> cmds;
    cmds.reserve(active.size());
    for (const auto* sub : active) cmds.push_back(sub->cmd);

    m_activeCommandCount = static_cast<uint32_t>(cmds.size());

    if (m_activeCommandCount == 0)
        return;

    m_bufferManager->populateBuffer(m_indirectBuffer, 0, {reinterpret_cast<const uint8_t*>(cmds.data()), cmds.size() * sizeof(DrawIndexedIndirectCommand)});
}

std::optional<SubMesh> MeshCollection::getSubMesh(uint32_t meshIndex) const  // TODO: probably will need fix to return & of subMesh
{
    if (meshIndex >= m_subMeshes.size())
        return std::nullopt;

    return m_subMeshes[meshIndex];
}

}  // namespace SE
