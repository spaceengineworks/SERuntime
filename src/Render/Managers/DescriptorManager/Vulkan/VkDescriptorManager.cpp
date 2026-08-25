#include "VkDescriptorManager.hpp"

#include "../../PipelineManager/IPipelineManager.hpp"

namespace SE
{

VkDescriptorManager::VkDescriptorManager() = default;

VkDescriptorManager::~VkDescriptorManager()
{
    if (!m_config || !m_config->device)
        return;

    for (auto& [hash, layout] : m_layoutCache) vkDestroyDescriptorSetLayout(*m_config->device, layout, nullptr);

    for (auto& entry : m_pools) vkDestroyDescriptorPool(*m_config->device, entry.pool, nullptr);
}

SeDescriptorID VkDescriptorManager::createDescriptor(DescriptorDesc desc)
{
    if (!m_config || !m_config->device || desc.bindings.empty())
        return SE_INVALID_DESCRIPTOR_ID;

    VkDescriptorSetLayout layout = getOrCreateLayout(desc);
    if (layout == VK_NULL_HANDLE)
        return SE_INVALID_DESCRIPTOR_ID;

    VkDescriptorSet set = allocateSet(layout);
    if (set == VK_NULL_HANDLE)
        return SE_INVALID_DESCRIPTOR_ID;

    std::vector<VkWriteDescriptorSet>   writes;
    std::vector<VkDescriptorImageInfo>  imageInfos;
    std::vector<VkDescriptorBufferInfo> bufferInfos;

    imageInfos.reserve(desc.bindings.size());
    bufferInfos.reserve(desc.bindings.size());

    for (const auto& binding : desc.bindings)
    {
        VkWriteDescriptorSet write {};
        write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet          = set;
        write.dstBinding      = binding.binding;
        write.dstArrayElement = 0;
        write.descriptorCount = binding.count;
        write.descriptorType  = toVkDescriptorType(binding.type);

        if (binding.type == DescriptorType::UNIFORM_BUFFER || binding.type == DescriptorType::STORAGE_BUFFER)
        {
            VkDescriptorBufferInfo bufferInfo {};
            bufferInfo.buffer = reinterpret_cast<VkBuffer>(binding.buffer);
            bufferInfo.offset = binding.offset;
            bufferInfo.range  = (binding.range == 0) ? VK_WHOLE_SIZE : binding.range;

            bufferInfos.push_back(bufferInfo);
            write.pBufferInfo = &bufferInfos.back();
        }
        else
        {
            VkDescriptorImageInfo imageInfo {};
            imageInfo.imageView   = static_cast<VkImageView>(binding.imageView);
            imageInfo.sampler     = static_cast<VkSampler>(binding.sampler);
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            imageInfos.push_back(imageInfo);
            write.pImageInfo = &imageInfos.back();
        }

        writes.push_back(write);
    }

    vkUpdateDescriptorSets(*m_config->device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

    Descriptor descriptor;
    descriptor.layout = layout;
    descriptor.set    = set;
    descriptor.pool   = m_pools.back().pool;

    if (!m_freeDescriptorIds.empty())
    {
        SeDescriptorID id = m_freeDescriptorIds.top();
        m_freeDescriptorIds.pop();

        m_descriptors[id] = descriptor;
        return id;
    }
    m_descriptors.push_back(descriptor);

    return m_nextId++;
}

SeResult VkDescriptorManager::destroyDescriptor(SeDescriptorID descriptorId)
{
    if (descriptorId == SE_INVALID_DESCRIPTOR_ID || descriptorId >= m_descriptors.size())
        return SE_FAILED_TO_DESTROY;

    Descriptor& descriptor = m_descriptors[descriptorId];

    if (descriptor.set == VK_NULL_HANDLE)
        return SE_FAILED_TO_DESTROY;

    vkFreeDescriptorSets(*m_config->device, descriptor.pool, 1, &descriptor.set);

    descriptor.set = VK_NULL_HANDLE;

    m_freeDescriptorIds.push(descriptorId);

    return SE_SUCCESS;
}

SeDescriptorHandle VkDescriptorManager::getDescriptor(SeDescriptorID descriptorId)
{
    if (descriptorId == SE_INVALID_DESCRIPTOR_ID || descriptorId >= m_descriptors.size())
        return nullptr;

    Descriptor& descriptor = m_descriptors[descriptorId];
    if (descriptor.set == VK_NULL_HANDLE)
        return nullptr;

    return static_cast<SeDescriptorHandle>(&descriptor);
}

/* --------- layout cache --------- */

size_t VkDescriptorManager::hashDescriptorDesc(const DescriptorDesc& desc)
{
    size_t hash = desc.bindings.size();
    for (const auto& b : desc.bindings)
    {
        hash ^= std::hash<uint32_t> {}(b.binding) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<uint32_t> {}(static_cast<uint32_t>(b.type)) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<uint32_t> {}(b.count) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<uint32_t> {}(b.stageFlags) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }
    return hash;
}

VkDescriptorSetLayout VkDescriptorManager::getOrCreateLayout(const DescriptorDesc& desc)
{
    size_t hash = hashDescriptorDesc(desc);

    auto it = m_layoutCache.find(hash);
    if (it != m_layoutCache.end())
        return it->second;

    std::vector<VkDescriptorSetLayoutBinding> vkBindings;
    vkBindings.reserve(desc.bindings.size());

    for (const auto& b : desc.bindings)
    {
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding            = b.binding;
        layoutBinding.descriptorType     = toVkDescriptorType(b.type);
        layoutBinding.descriptorCount    = b.count;
        layoutBinding.stageFlags         = toVkShaderStages(b.stageFlags);
        layoutBinding.pImmutableSamplers = nullptr;

        vkBindings.push_back(layoutBinding);
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo {};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(vkBindings.size());
    layoutInfo.pBindings    = vkBindings.data();

    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    if (vkCreateDescriptorSetLayout(*m_config->device, &layoutInfo, nullptr, &layout) != VK_SUCCESS)
        return VK_NULL_HANDLE;

    m_layoutCache[hash] = layout;
    return layout;
}

VkDescriptorPool VkDescriptorManager::createNewPool()
{
    VkDescriptorPoolSize sizes[] = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, SETS_PER_POOL},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SETS_PER_POOL},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, SETS_PER_POOL},
    };

    VkDescriptorPoolCreateInfo poolInfo {};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(std::size(sizes));
    poolInfo.pPoolSizes    = sizes;
    poolInfo.maxSets       = SETS_PER_POOL;
    poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    VkDescriptorPool pool = VK_NULL_HANDLE;
    if (vkCreateDescriptorPool(*m_config->device, &poolInfo, nullptr, &pool) != VK_SUCCESS)
        return VK_NULL_HANDLE;

    m_pools.push_back({pool, 0});
    return pool;
}

VkDescriptorSet VkDescriptorManager::allocateSet(VkDescriptorSetLayout layout)
{
    if (m_pools.empty() || m_pools.back().allocatedSets >= SETS_PER_POOL)
    {
        if (createNewPool() == VK_NULL_HANDLE)
            return VK_NULL_HANDLE;
    }

    VkDescriptorSetAllocateInfo allocInfo {};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = m_pools.back().pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts        = &layout;

    VkDescriptorSet set = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(*m_config->device, &allocInfo, &set) != VK_SUCCESS)
        return VK_NULL_HANDLE;

    m_pools.back().allocatedSets++;
    return set;
}

VkDescriptorType VkDescriptorManager::toVkDescriptorType(DescriptorType type)
{
    switch (type)
    {
        case DescriptorType::UNIFORM_BUFFER:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case DescriptorType::STORAGE_BUFFER:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case DescriptorType::COMBINED_IMAGE_SAMPLER:
            return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case DescriptorType::SAMPLED_IMAGE:
            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case DescriptorType::STORAGE_IMAGE:
            return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        default:
            return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    }
}

VkShaderStageFlags VkDescriptorManager::toVkShaderStages(uint32_t stageFlags)
{
    VkShaderStageFlags flags = 0;
    if (stageFlags & static_cast<uint32_t>(ShaderStage::VERTEX))
        flags |= VK_SHADER_STAGE_VERTEX_BIT;
    if (stageFlags & static_cast<uint32_t>(ShaderStage::FRAGMENT))
        flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
    if (stageFlags & static_cast<uint32_t>(ShaderStage::COMPUTE))
        flags |= VK_SHADER_STAGE_COMPUTE_BIT;
    return flags;
}

}  // namespace SE