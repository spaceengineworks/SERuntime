#ifndef VK_DESCRIPTOR_MANAGER_HPP
#define VK_DESCRIPTOR_MANAGER_HPP

#include <functional>
#include <queue>
#include <unordered_map>
#include <vector>

#include "../../../RHI/Vulkan/VulkanConfig.hpp"
#include "../IDescriptorManager.hpp"

namespace SE
{

struct Descriptor
{
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    VkDescriptorSet       set    = VK_NULL_HANDLE;
    VkDescriptorPool      pool   = VK_NULL_HANDLE;
};

class VkDescriptorManager : public IDescriptorManager
{
   public:
    VkDescriptorManager();
    ~VkDescriptorManager() override;

    virtual SeDescriptorID     createDescriptor(DescriptorDesc desc) override;
    virtual SeResult           updateDescriptor(SeDescriptorID descriptorId, DescriptorDesc bind) override;
    virtual SeResult           destroyDescriptor(SeDescriptorID descriptorId) override;
    virtual SeDescriptorHandle getDescriptor(SeDescriptorID descriptorId) override;

    void setConfig(SharedVulkanConfig* config)
    {
        if (config)
            m_config = config;
    }

   private:
    /* using types */
    using FreeDescriptorIdHeap = std::priority_queue<SeDescriptorID, std::vector<SeDescriptorID>, std::greater<SeDescriptorID>>;

    struct PoolEntry
    {
        VkDescriptorPool pool          = VK_NULL_HANDLE;
        uint32_t         allocatedSets = 0;
    };

    /* --------------- */

    SharedVulkanConfig* m_config = nullptr;

    std::vector<Descriptor> m_descriptors;
    FreeDescriptorIdHeap    m_freeDescriptorIds;
    SeDescriptorID          m_nextId = 0;

    std::unordered_map<size_t, VkDescriptorSetLayout> m_layoutCache;

    std::vector<PoolEntry> m_pools;

    static constexpr uint32_t SETS_PER_POOL = 64;

    VkDescriptorSetLayout getOrCreateLayout(const DescriptorDesc& desc);
    VkDescriptorSet       allocateSet(VkDescriptorSetLayout layout);
    VkDescriptorPool      createNewPool();

    static size_t hashDescriptorDesc(const DescriptorDesc& desc);

    static VkDescriptorType   toVkDescriptorType(DescriptorType type);
    static VkShaderStageFlags toVkShaderStages(uint32_t stageFlags);
};

}  // namespace SE

#endif /* VK_DESCRIPTOR_MANAGER_HPP */