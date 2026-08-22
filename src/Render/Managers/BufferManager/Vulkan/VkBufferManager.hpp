#ifndef VK_BUFFER_MANAGER_H
#define VK_BUFFER_MANAGER_H

#include <queue>
#include <vector>

#include "../../../RHI/Vulkan/VulkanConfig.hpp"
#include "../IBufferManager.hpp"
#include "vk_mem_alloc.h"

namespace SE::Render::Buffer
{

struct Buffer
{
    VkBuffer          buffer     = VK_NULL_HANDLE;
    VmaAllocation     allocation = nullptr;
    VmaAllocationInfo allocationInfo {};

    BufferDesc   desc;
    VkDeviceSize populated = 0;
};

class VkBufferManager : public IBufferManager
{
   public:
    VkBufferManager();
    ~VkBufferManager() override;

    SeBufferID     createBuffer(BufferDesc desc) override;
    SeResult       populateBuffer(SeBufferID bufferId, uint32_t offset, std::span<const uint8_t> data) override;
    SeResult       destroyBuffer(SeBufferID bufferId) override;
    SeBufferHandle getBufferHandle(SeBufferID bufferId) override;

    void setConfig(SharedVulkanConfig* config)
    {
        if (config)
            m_config = config;
    }

   private:
    /* using types */
    using FreeBufferIdHeap = std::priority_queue<SeBufferID, std::vector<SeBufferID>, std::greater<SeBufferID>>;

    /* --------------- */

    SharedVulkanConfig* m_config = nullptr;

    std::vector<Buffer> m_buffers;
    FreeBufferIdHeap    m_freeBufferIds;
    SeBufferID          m_nextId = 0;

    SeResult resizeBuffer(SeBufferID bufferId, VkDeviceSize newSize);

    SeResult        copyOnGpu(VkBuffer src, VkBuffer dst, VkDeviceSize size);
    VkCommandBuffer beginSingleTimeCommands();
    void            endSingleTimeCommands(VkCommandBuffer commandBuffer);

    static VmaAllocationCreateInfo makeAllocCreateInfo(BufferMemoryType memoryType);
    static VkBufferUsageFlags      toVkBufferUsage(BufferUsage usage);
};

}  // namespace SE::Render::Buffer

#endif /* VK_BUFFER_MANAGER_H */