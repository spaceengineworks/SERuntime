#include "SERuntime.hpp"

#include <tracy/Tracy.hpp>

#include "Render/RHI/Vulkan/VulkanDevice.hpp"

namespace SE
{

SERuntime::SERuntime(SeRender render)
{
    if (render == SeRender::Vulkan)
        m_context = std::make_unique<VulkanDevice>();

    m_frameGraph = std::make_unique<FrameGraph>(m_context.get());
    if (!m_frameGraph)
        throw std::runtime_error("frame graph not init.");
}

SERuntime::~SERuntime() = default;

SeRenderHandle SERuntime::SeAskConfig()
{
    if (!m_context)
        return nullptr;
    return m_context->passConfig();
}

SeResult SERuntime::initViewPort(uint32_t width, uint32_t height)
{
    m_context->CreateViewPortImage(width, height);
    m_context->createOffscreenRenderPass();
    m_context->createOffscreenFramebuffer();
    m_context->viewPortCommandBuffer();
    m_context->createSyncObjects();

    // PassClearParameters* clear = new PassClearParameters();
    PassClearParameters* clear = m_frameAllocator.allocateFrameParams<PassClearParameters>();
    clear->m_clearColor[0]     = 0.1f;
    clear->m_clearColor[1]     = 0.15f;
    clear->m_clearColor[2]     = 0.2f;
    clear->m_clearColor[3]     = 1.0f;

    m_frameGraph->add("clear", GPUFlags::Render, clear,
                      [clear](RHI* ctx)
                      {
                          static float speed[3] = {0.00005f, 0.0002f, 0.0003f};

                          for (int i = 0; i < 3; ++i)
                          {
                              clear->m_clearColor[i] += speed[i];

                              if (clear->m_clearColor[i] >= 1.0f)
                              {
                                  clear->m_clearColor[i] = 1.0f;
                                  speed[i]               = -speed[i];
                              }
                              else if (clear->m_clearColor[i] <= 0.0f)
                              {
                                  clear->m_clearColor[i] = 0.0f;
                                  speed[i]               = -speed[i];
                              }
                          }

                          ctx->setClearColor(clear->m_clearColor[0], clear->m_clearColor[1], clear->m_clearColor[2]);

                          SeResourseHandle viewportImage = ctx->getViewportImageHandle(*ctx->getCurrentFrameIndex());

                          BarrierDesc barrier {};
                          barrier.resourse = viewportImage;
                          barrier.type     = ResourseType::Image;

                          barrier.prevAccesses = {THSVS_ACCESS_FRAGMENT_SHADER_READ_SAMPLED_IMAGE_OR_UNIFORM_TEXEL_BUFFER};
                          barrier.nextAccesses = {THSVS_ACCESS_COLOR_ATTACHMENT_WRITE};

                          barrier.prevLayout = THSVS_IMAGE_LAYOUT_OPTIMAL;
                          barrier.nextLayout = THSVS_IMAGE_LAYOUT_OPTIMAL;

                          barrier.baseMipLevel   = 0;
                          barrier.levelCount     = 1;
                          barrier.baseArrayLayer = 0;
                          barrier.layerCount     = 1;

                          ctx->insertPipelineBarrier(&barrier);

                          ctx->beginRenderPass();
                      });

    m_frameGraph->build();

    return SeResult::SE_SUCCESS;
}

void SERuntime::updateAndRender()
{
    if (m_context)
    {
        m_frameAllocator.reset();
        m_context->updateAndRender(m_frameGraph.get());
    }
}

SeTextureHandle SERuntime::getViewportTex(uint32_t currentFrame) const
{
    if (!m_context)
        return VK_NULL_HANDLE;

    return m_context->getViewportTex(currentFrame);
}

}  // namespace SE