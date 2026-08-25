#include "VkBufferManager.hpp"

#include <stdexcept>

namespace SE
{

VkBufferManager::VkBufferManager()
{
}

VkBufferManager::~VkBufferManager()
{
    for (Buffer& buf : m_buffers)
    {
        if (buf.buffer != VK_NULL_HANDLE)
            vmaDestroyBuffer(*m_config->allocator, buf.buffer, buf.allocation);
    }
}

SeBufferID VkBufferManager::createBuffer(BufferDesc desc)
{
    VkBufferCreateInfo bufferInfo {};
    bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size        = desc.size;
    bufferInfo.usage       = toVkBufferUsage(desc.usage);
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = makeAllocCreateInfo(desc.memoryType);

    Buffer newBuffer {};
    newBuffer.desc  = desc;
    VkResult result = vmaCreateBuffer(*m_config->allocator, &bufferInfo, &allocInfo, &newBuffer.buffer, &newBuffer.allocation, &newBuffer.allocationInfo);

    if (result != VK_SUCCESS)
        return SE_INVALID_BUFFER_ID;

    SeBufferID id;
    if (!m_freeBufferIds.empty())
    {
        id = m_freeBufferIds.top();
        m_freeBufferIds.pop();
        m_buffers[id] = newBuffer;
    }
    else
    {
        id = m_nextId++;
        m_buffers.push_back(newBuffer);
    }

    return id;
}

SeResult VkBufferManager::populateBuffer(SeBufferID bufferId, uint32_t offset, std::span<const uint8_t> data)
{
    if (bufferId == SE_INVALID_BUFFER_ID || bufferId >= m_buffers.size())
        return SE_FAILED;

    Buffer& buf = m_buffers[bufferId];

    if (offset + data.size() > buf.desc.size)
    {
        VkDeviceSize newSize      = std::max(buf.desc.size * 2, (VkDeviceSize) (offset + data.size()));
        SeResult     resizeResult = resizeBuffer(bufferId, newSize);
        if (resizeResult != SE_SUCCESS)
            return SE_FAILED;
    }

    if (buf.desc.memoryType == BufferMemoryType::HOST_VISIBLE)
    {
        VkResult result = vmaCopyMemoryToAllocation(*m_config->allocator, data.data(), buf.allocation, offset, data.size());
        if (result != VK_SUCCESS)
            return SE_FAILED;
    }
    else
    {
        VkBufferCreateInfo stagingInfo {};
        stagingInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingInfo.size        = data.size();
        stagingInfo.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo stagingAllocInfo {};
        stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer          stagingBuffer;
        VmaAllocation     stagingAllocation;
        VmaAllocationInfo stagingAllocationInfo {};

        if (vmaCreateBuffer(*m_config->allocator, &stagingInfo, &stagingAllocInfo, &stagingBuffer, &stagingAllocation, &stagingAllocationInfo) != VK_SUCCESS)
            return SE_FAILED;

        memcpy(stagingAllocationInfo.pMappedData, data.data(), data.size());

        VkCommandBuffer cmd = beginSingleTimeCommands();

        VkBufferCopy region {};
        region.srcOffset = 0;
        region.dstOffset = offset;
        region.size      = data.size();
        vkCmdCopyBuffer(cmd, stagingBuffer, buf.buffer, 1, &region);

        endSingleTimeCommands(cmd);

        vmaDestroyBuffer(*m_config->allocator, stagingBuffer, stagingAllocation);
    }

    buf.populated = std::max(buf.populated, (VkDeviceSize) (offset + data.size()));
    return SE_SUCCESS;
}

SeResult VkBufferManager::destroyBuffer(SeBufferID bufferId)
{
    if (bufferId == SE_INVALID_BUFFER_ID || bufferId >= m_buffers.size())
        return SE_FAILED_TO_DESTROY;

    Buffer& buf = m_buffers[bufferId];

    if (buf.buffer == VK_NULL_HANDLE)
        return SE_FAILED_TO_DESTROY;

    vmaDestroyBuffer(*m_config->allocator, buf.buffer, buf.allocation);

    buf.buffer     = VK_NULL_HANDLE;
    buf.allocation = nullptr;

    m_freeBufferIds.push(bufferId);

    return SE_SUCCESS;
}

SeBufferHandle VkBufferManager::getBufferHandle(SeBufferID bufferId)
{
    if (bufferId == SE_INVALID_BUFFER_ID || bufferId >= m_buffers.size())
        return nullptr;

    Buffer& buf = m_buffers[bufferId];
    if (buf.buffer == VK_NULL_HANDLE)
        return nullptr;

    return static_cast<SeBufferHandle>(&buf);
}

/* Note: this method do not check valid bufferId it assumes it was checked before because this method used only enternaly */
SeResult VkBufferManager::resizeBuffer(SeBufferID bufferId, VkDeviceSize newSize)
{
    Buffer& buf = m_buffers[bufferId];

    VkBufferCreateInfo bufferInfo {};
    bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size        = newSize;
    bufferInfo.usage       = toVkBufferUsage(buf.desc.usage) | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = makeAllocCreateInfo(buf.desc.memoryType);

    Buffer newBuffer {};
    newBuffer.desc      = buf.desc;
    newBuffer.desc.size = newSize;

    VkResult result = vmaCreateBuffer(*m_config->allocator, &bufferInfo, &allocInfo, &newBuffer.buffer, &newBuffer.allocation, &newBuffer.allocationInfo);
    if (result != VK_SUCCESS)
        return SE_FAILED;

    if (buf.populated > 0)
    {
        if (buf.desc.memoryType == BufferMemoryType::HOST_VISIBLE)
        {
            void* oldMapped = nullptr;
            if (vmaMapMemory(*m_config->allocator, buf.allocation, &oldMapped) != VK_SUCCESS)
            {
                vmaDestroyBuffer(*m_config->allocator, newBuffer.buffer, newBuffer.allocation);
                return SE_FAILED;
            }
            vmaCopyMemoryToAllocation(*m_config->allocator, oldMapped, newBuffer.allocation, 0, buf.populated);
            vmaUnmapMemory(*m_config->allocator, buf.allocation);
        }
        else
        {
            if (copyOnGpu(buf.buffer, newBuffer.buffer, buf.populated) != SE_SUCCESS)
            {
                vmaDestroyBuffer(*m_config->allocator, newBuffer.buffer, newBuffer.allocation);
                return SE_FAILED;
            }
        }
    }

    vmaDestroyBuffer(*m_config->allocator, buf.buffer, buf.allocation);

    buf.buffer         = newBuffer.buffer;
    buf.allocation     = newBuffer.allocation;
    buf.allocationInfo = newBuffer.allocationInfo;
    buf.desc.size      = newSize;
    buf.populated      = buf.populated;

    return SE_SUCCESS;
}

VkCommandBuffer VkBufferManager::beginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool        = *(m_config->commandPool);
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(*(m_config->device), &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VkBufferManager::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo {};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer;

    vkQueueSubmit(*(m_config->graphicsQueue), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(*(m_config->graphicsQueue));

    vkFreeCommandBuffers(*(m_config->device), *(m_config->commandPool), 1, &commandBuffer);
}

SeResult VkBufferManager::copyOnGpu(VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
    if (!m_config || !m_config->device || !m_config->commandPool || !m_config->graphicsQueue)
        return SE_FAILED;

    VkCommandBuffer cmd = beginSingleTimeCommands();

    VkBufferCopy region {};
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size      = size;

    vkCmdCopyBuffer(cmd, src, dst, 1, &region);

    endSingleTimeCommands(cmd);
    return SE_SUCCESS;
}

VmaAllocationCreateInfo VkBufferManager::makeAllocCreateInfo(BufferMemoryType memoryType)
{
    VmaAllocationCreateInfo allocCreateInfo {};

    switch (memoryType)
    {
        case BufferMemoryType::HOST_VISIBLE:
            allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
            allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
            break;

        case BufferMemoryType::DEVICE_LOCAL:
            allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            allocCreateInfo.flags = 0;
            break;

        default:
            throw std::runtime_error("Unknown buffer memory type.");
    }

    return allocCreateInfo;
}

VkBufferUsageFlags VkBufferManager::toVkBufferUsage(BufferUsage usage)
{
    VkBufferUsageFlags flags = 0;

    if ((usage & BufferUsage::TRANSFER_SRC) != BufferUsage::NONE)
        flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    if ((usage & BufferUsage::TRANSFER_DST) != BufferUsage::NONE)
        flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    if ((usage & BufferUsage::UNIFORM) != BufferUsage::NONE)
        flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    if ((usage & BufferUsage::STORAGE) != BufferUsage::NONE)
        flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    if ((usage & BufferUsage::INDEX) != BufferUsage::NONE)
        flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    if ((usage & BufferUsage::VERTEX) != BufferUsage::NONE)
        flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    if ((usage & BufferUsage::INDIRECT) != BufferUsage::NONE)
        flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

    return flags;
}

}  // namespace SE