#ifndef VULKANDEVICE_HPP

#include <vulkan/vulkan.h>

#include <array>
#include <memory>
#include <stdexcept>
#include <string>
#include <tracy/Tracy.hpp>
#include <vector>

#include "RHI/RHI.hpp"
#include "VulkanConfig.hpp"

namespace SE
{
class VulkanDevice : public RHI
{
   public:
    VulkanDevice();
    ~VulkanDevice() override;

    SeRenderHandle passConfig() override
    {
        return static_cast<SeRenderHandle>(m_config.get());
    }

    void            CreateViewPortImage(uint32_t width, uint32_t height) override;
    void            recreateViewPort() override;
    void            createOffscreenFramebuffer() override;
    void            viewPortCommandBuffer() override;
    SeTextureHandle getViewportTex(uint32_t frameIndex) override;

    void updateAndRender() override;
    void createOffscreenRenderPass() override;
    void createSyncObjects() override;
    void cleanUp() override;

    const uint32_t* getCurrentFrameIndex() override;
    uint32_t        getFramesInFlightCount() const
    {
        return *m_config->frame_in_flight;
    }

    SeResult needVPResize(const uint32_t* width, const uint32_t* height) override
    {
        return ((*width != m_viewPortWidth) || (*height != m_viewPortHeight)) ? SeResult::SE_RESIZED
                                                                              : SE_SUCCESS;
    }

    void resizeVP(const uint32_t* width, const uint32_t* height)
    {
        m_viewPortWidth  = (*width);
        m_viewPortHeight = (*height);
        m_VPResized      = true;
    }

    void setClearColor(float r, float g, float b) override
    {
        m_clearColor[0] = r;
        m_clearColor[1] = g;
        m_clearColor[2] = b;
    }

   private:
    std::unique_ptr<SharedVulkanConfig> m_config = nullptr;

    float m_clearColor[4] = {0.1f, 0.15f, 0.2f, 1.0f};

    struct ViewPort
    {
        VkImage         image         = VK_NULL_HANDLE;
        VkDeviceMemory  imageMemory   = VK_NULL_HANDLE;
        VkImageView     imageView     = VK_NULL_HANDLE;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    };

    std::vector<ViewPort>        m_viewPort;
    std::vector<VkCommandBuffer> m_commandBuffers;
    std::vector<VkFramebuffer>   m_viewPortFramebuffers;

    uint32_t m_viewPortWidth = 0, m_viewPortHeight = 0;
    bool     m_VPResized = false;

    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkSampler    m_sampler    = VK_NULL_HANDLE;

    std::vector<VkFence> m_inFlightFences;

    uint32_t  findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void      recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    VkSampler getOrCreateDefaultSampler();

    void cleanViewPort();
};
}  // namespace SE

#endif /* VULKANDEVICE_HPP */