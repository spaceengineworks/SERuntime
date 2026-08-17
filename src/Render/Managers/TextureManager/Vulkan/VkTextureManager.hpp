#ifndef VK_TEXTURE_MANAGER_H
#define VK_TEXTURE_MANAGER_H

#include <queue>
#include <vector>

#include "../../../RHI/Vulkan/VulkanConfig.hpp"
#include "../ITextureManager.hpp"
#include "vk_mem_alloc.h"

namespace SE::Render::Texture
{

struct TextureData
{
    VkImage     image;
    VkImageView imageView;
    VkSampler   sampler;
};

struct Texture
{
    TextureData data;

    VmaAllocation allocation;
    bool          isValid = false;
};

class VkTextureManager : public ITextureManager
{
   public:
    VkTextureManager();
    ~VkTextureManager() override;

    SeTextureID     createTexture(TextureDesc desc) override;
    SeResult        destroyTexture(SeTextureID textureId) override;
    SeTextureHandle getTextureHandle(SeTextureID textureId) override;

    void setConfig(SharedVulkanConfig* config)
    {
        if (config)
            m_config = config;
    }

   private:
    /* using types */
    using FreeTextureIdHeap = std::priority_queue<SeTextureID, std::vector<SeTextureID>, std::greater<SeTextureID>>;

    /* --------------- */

    SharedVulkanConfig* m_config = nullptr;

    std::vector<Texture> m_textures;
    FreeTextureIdHeap    m_freeTextureIds;
    SeTextureID          m_nextId = 0;

    enum SamplerType
    {
        LINEAR,
        SAMPLER_COUNT
    };
    std::vector<VkSampler> m_textureSamplers = std::vector<VkSampler>(SAMPLER_COUNT, VK_NULL_HANDLE);

    /* Methods to change layout one time */
    SeResult        loadTexture(VkImage image, TextureDesc* desc);
    void            copyBufferToImage(VkBuffer buffer, VkImage image, TextureDesc* desc);
    VkCommandBuffer beginSingleTimeCommands();
    void            endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void            transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    VkSampler       getOrCreateDefaultSampler(SamplerType type);
};

}  // namespace SE::Render::Texture

#endif /* VK_TEXTURE_MANAGER_H */