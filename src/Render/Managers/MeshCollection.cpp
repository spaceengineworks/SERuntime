#include "MeshCollection.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>

namespace SE
{

MeshCollection::MeshCollection(IBufferManager* bufferMgr, SharedVulkanConfig* config, size_t maxVertexBytes, size_t maxIndexBytes) : m_config(config), m_bufferManager(bufferMgr)
{
    assert(m_bufferManager && "BufferManager cannot be null!");
    assert(m_config && m_config->frame_in_flight && "Config with frame_in_flight must be set before creating a MeshCollection!");

    BufferDesc vDesc {};
    vDesc.size       = maxVertexBytes;
    vDesc.usage      = BufferUsage::VERTEX | BufferUsage::TRANSFER_DST;
    vDesc.memoryType = BufferMemoryType::DEVICE_LOCAL;
    m_vertexBuffer   = m_bufferManager->createBuffer(vDesc);

    BufferDesc iDesc {};
    iDesc.size       = maxIndexBytes;
    iDesc.usage      = BufferUsage::INDEX | BufferUsage::TRANSFER_DST;
    iDesc.memoryType = BufferMemoryType::HOST_VISIBLE;
    m_indexBuffer    = m_bufferManager->createBuffer(iDesc);

    BufferDesc indirectDesc {};
    indirectDesc.size       = DEFAULT_RESERVED_DRAW_COMMANDS * sizeof(DrawIndexedIndirectCommand);
    indirectDesc.usage      = BufferUsage::INDIRECT | BufferUsage::TRANSFER_DST;
    indirectDesc.memoryType = BufferMemoryType::HOST_VISIBLE;
    m_indirectBuffer        = m_bufferManager->createBuffer(indirectDesc);

    const uint32_t framesInFlight = static_cast<uint32_t>(*m_config->frame_in_flight);

    m_storageBuffer.resize(framesInFlight);
    m_transformsBuffer.resize(framesInFlight);

    for (uint32_t i = 0; i < framesInFlight; i++)
    {
        BufferDesc storageDesc {};
        storageDesc.size       = DEFAULT_RESERVED_DATA_COMMANDS * sizeof(MeshInstanceData);
        storageDesc.usage      = BufferUsage::STORAGE | BufferUsage::TRANSFER_DST;
        storageDesc.memoryType = BufferMemoryType::HOST_VISIBLE;
        m_storageBuffer[i]     = m_bufferManager->createBuffer(storageDesc);

        BufferDesc transformDesc {};
        transformDesc.size       = DEFAULT_RESERVED_TRANSFORM_MATRIX * sizeof(glm::mat4);
        transformDesc.usage      = BufferUsage::STORAGE | BufferUsage::TRANSFER_DST;
        transformDesc.memoryType = BufferMemoryType::HOST_VISIBLE;
        m_transformsBuffer[i]    = m_bufferManager->createBuffer(transformDesc);
    }
}

SeMeshID MeshCollection::addMesh(std::span<const uint8_t> vertexData, std::span<const uint8_t> indexData, uint32_t indexCount, uint32_t vertexStride, SeSortKey sortKey)
{
    const uint32_t commandIndex = static_cast<uint32_t>(m_subMeshes.size());

    SubMesh sub {};
    sub.vertexOffsetBytes = m_currentVertexOffset;
    sub.indexOffsetBytes  = m_currentIndexOffset;
    sub.sortKey           = sortKey;
    sub.isActive          = true;

    sub.cmd.indexCount    = indexCount;
    sub.cmd.instanceCount = 1;
    sub.cmd.firstIndex    = m_currentIndexOffset / sizeof(uint32_t);
    sub.cmd.vertexOffset  = static_cast<int32_t>(m_currentVertexOffset / vertexStride);
    sub.cmd.firstInstance = commandIndex;

    sub.meshData.transformIndex = commandIndex;

    if (m_bufferManager->populateBuffer(m_vertexBuffer, m_currentVertexOffset, vertexData) != SE_SUCCESS
        || m_bufferManager->populateBuffer(m_indexBuffer, m_currentIndexOffset, indexData) != SE_SUCCESS)
        return SE_INVALID_MESH_ID;

    std::span<const uint8_t> meshDataBytes {reinterpret_cast<const uint8_t*>(&sub.meshData), sizeof(MeshInstanceData)};

    for (size_t i = 0; i < m_storageBuffer.size(); i++)
    {
        if (m_bufferManager->populateBuffer(m_storageBuffer[i], commandIndex * sizeof(MeshInstanceData), meshDataBytes) != SE_SUCCESS)
            return SE_INVALID_MESH_ID;
    }

    m_currentVertexOffset += static_cast<uint32_t>(vertexData.size());
    m_currentIndexOffset += static_cast<uint32_t>(indexData.size());

    m_subMeshes.push_back(sub);
    return commandIndex;
}

SeResult MeshCollection::updateMeshData(WhichType update, SeMeshID meshId, std::span<const uint8_t> data)
{
    if (meshId == SE_INVALID_MESH_ID || meshId >= m_subMeshes.size())
        return SE_FAILED;

    switch (update)
    {
        case WhichType::TRANSFORMS:
        {
            if (data.size() != sizeof(glm::mat4))
                return SE_FAILED;

            uint32_t slot = m_subMeshes[meshId].meshData.transformIndex;

            for (size_t i = 0; i < m_transformsBuffer.size(); i++)
            {
                if (m_bufferManager->populateBuffer(m_transformsBuffer[i], slot * sizeof(glm::mat4), data, UploadMode::Async) != SE_SUCCESS)
                    return SE_FAILED;
            }
            return SE_SUCCESS;
        }

        case WhichType::STORAGE:
        {
            if (data.size() != sizeof(MeshInstanceData))
                return SE_FAILED;

            std::memcpy(&m_subMeshes[meshId].meshData, data.data(), sizeof(MeshInstanceData));

            for (size_t i = 0; i < m_storageBuffer.size(); i++)
            {
                if (m_bufferManager->populateBuffer(m_storageBuffer[i], meshId * sizeof(MeshInstanceData), data, UploadMode::Async) != SE_SUCCESS)
                    return SE_FAILED;
            }
            return SE_SUCCESS;
        }

        default:
            return SE_FAILED;
    }
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

    std::vector<DrawIndexedIndirectCommand> cmds;
    cmds.reserve(active.size());
    for (const auto* sub : active) cmds.push_back(sub->cmd);

    m_activeCommandCount = static_cast<uint32_t>(cmds.size());

    if (m_activeCommandCount == 0)
        return;

    m_bufferManager->populateBuffer(m_indirectBuffer, 0, {reinterpret_cast<const uint8_t*>(cmds.data()), cmds.size() * sizeof(DrawIndexedIndirectCommand)});
}

std::optional<SubMesh> MeshCollection::getSubMesh(uint32_t meshIndex) const
{
    if (meshIndex >= m_subMeshes.size())
        return std::nullopt;

    return m_subMeshes[meshIndex];
}

}  // namespace SE