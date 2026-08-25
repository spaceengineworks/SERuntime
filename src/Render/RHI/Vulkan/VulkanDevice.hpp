#ifndef VULKANDEVICE_HPP
#define VULKANDEVICE_HPP

#include <vulkan/vulkan.h>

#include <array>
#include <memory>
#include <stdexcept>
#include <string>
#include <tracy/Tracy.hpp>
#include <vector>

#include "RHI/RHI.hpp"
#include "VulkanConfig.hpp"
#include "vk_mem_alloc.h"

//

#include "../../../Render/Managers/BufferManager/Vulkan/VkBufferManager.hpp"
#include "../../../Render/Managers/DescriptorManager/Vulkan/VkDescriptorManager.hpp"
#include "../../../Render/Managers/MeshCollection.hpp"
#include "../../../Render/Managers/PipelineManager/Vulkan/VkPipelineManager.hpp"

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

    void             CreateViewPortImage(uint32_t width, uint32_t height) override;
    void             recreateViewPort() override;
    void             createOffscreenFramebuffer() override;
    void             viewPortCommandBuffer() override;
    SeTextureHandle  getViewportTex(uint32_t frameIndex) override;
    SeResourseHandle getViewportImageHandle(uint32_t frameIndex) override;

    void updateAndRender(FrameGraph* frameGraph) override;
    void createOffscreenRenderPass() override;
    void createSyncObjects() override;
    void cleanUp() override;

    const uint32_t* getCurrentFrameIndex() override;
    uint32_t        getFramesInFlightCount() const
    {
        return *m_config->frame_in_flight;
    }

    VkRenderPass* getRenderPassPtr()
    {
        return &m_renderPass;
    }

    SeResult needVPResize(const uint32_t* width, const uint32_t* height) override
    {
        return ((*width != m_viewPortWidth) || (*height != m_viewPortHeight)) ? SeResult::SE_RESIZED : SE_SUCCESS;
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

    /* TODO: temp */
    void insertPipelineBarrier(BarrierDesc* barrier) override;

    void beginRenderPass() override;

    /* frame graph methods */
    virtual void endRenderPass()
    {
        vkCmdEndRenderPass(m_currCmdBuff);
    }

    virtual void DrawIndexedIndirect(void* collection)
    {
        MeshCollection* coll   = (MeshCollection*) collection;
        Buffer*         buffer = static_cast<Buffer*>(coll->getIndirectBufferhandle());

        vkCmdDrawIndexedIndirect(m_currCmdBuff, buffer->buffer, 0, coll->getAvtiveMeshes(), sizeof(VkDrawIndexedIndirectCommand));
    }

    virtual void callDraw()
    {
        vkCmdDraw(m_currCmdBuff, 12, 1, 0, 0);
    }

    virtual void bindPipe(void* handle)
    {
        auto* pipeline = static_cast<Pipeline*>(handle);
        if (!pipeline)
            return;

        vkCmdBindPipeline(m_currCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
    }

    virtual void bindVertexBuffers(void* handle)
    {
        MeshCollection* coll = static_cast<MeshCollection*>(handle);

        Buffer* buffer = static_cast<Buffer*>(coll->getVertexBufferHandle());

        VkDeviceSize offset = 0;

        vkCmdBindVertexBuffers(m_currCmdBuff, 0, 1, &buffer->buffer, &offset);
    }

    virtual void bindIndexBuffer(void* handle)
    {
        MeshCollection* coll = static_cast<MeshCollection*>(handle);

        Buffer* buffer = static_cast<Buffer*>(coll->getIndexBufferHandle());

        vkCmdBindIndexBuffer(m_currCmdBuff, buffer->buffer, 0, VK_INDEX_TYPE_UINT32);
    }

    virtual void setViewport()
    {
        VkViewport viewport {};
        viewport.x        = 0.0f;
        viewport.y        = 0.0f;
        viewport.width    = (float) m_viewPortWidth;
        viewport.height   = (float) m_viewPortHeight;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(m_currCmdBuff, 0, 1, &viewport);
    }

    virtual void setScissor()
    {
        VkRect2D scissor {};
        scissor.offset = {0, 0};
        scissor.extent = {m_viewPortWidth, m_viewPortHeight};
        vkCmdSetScissor(m_currCmdBuff, 0, 1, &scissor);
    }

    virtual void bindDescriptorSet(void* handle, void* pipe)
    {
        if (!handle)
            return;

        auto* pipeline   = static_cast<Pipeline*>(pipe);
        auto* descriptor = static_cast<Descriptor*>(handle);
        vkCmdBindDescriptorSets(m_currCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipelineLayout, 0, 1, &descriptor->set, 0, nullptr);
    }

    void pushConstants(void* pipelineHandle, uint32_t stage, uint32_t offset, uint32_t size, const void* data)
    {
        auto* pipeline = static_cast<Pipeline*>(pipelineHandle);
        if (!pipeline)
            return;

        vkCmdPushConstants(m_currCmdBuff, pipeline->pipelineLayout, (VkShaderStageFlags) stage, offset, size, data);
    }
    /*---------------------*/

   private:
    std::unique_ptr<SharedVulkanConfig> m_config = nullptr;

    float m_clearColor[4] = {0.1f, 0.15f, 0.2f, 1.0f};

    std::vector<ViewPort>        m_viewPort;
    std::vector<VkCommandBuffer> m_commandBuffers;
    std::vector<VkFramebuffer>   m_viewPortFramebuffers;

    uint32_t m_viewPortWidth = 0, m_viewPortHeight = 0;
    bool     m_VPResized = false;

    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkSampler    m_sampler    = VK_NULL_HANDLE;

    std::vector<VkFence> m_inFlightFences;

    VkCommandBuffer m_currCmdBuff = VK_NULL_HANDLE;

    uint32_t  findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void      recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, FrameGraph* frameGraph);
    VkSampler getOrCreateDefaultSampler();

    void cleanViewPort();
};
}  // namespace SE

#endif /* VULKANDEVICE_HPP */