#include "SERuntime.hpp"

#include <tracy/Tracy.hpp>

#include "Render/RHI/Vulkan/VulkanDevice.hpp"

namespace SE
{

SERuntine::SERuntine(SeRender render)
{
    if (render == SeRender::Vulkan)
        m_hardware = std::make_unique<VulkanDevice>();
}

SERuntine::~SERuntine() = default;

SeRenderHandle SERuntine::SeAskConfig()
{
    if (!m_hardware)
        return nullptr;
    return m_hardware->passConfig();
}

SeResult SERuntine::initViewPort(uint32_t width, uint32_t height)
{
    m_hardware->CreateViewPortImage(width, height);
    m_hardware->createOffscreenRenderPass();
    m_hardware->createOffscreenFramebuffer();
    m_hardware->viewPortCommandBuffer();
    m_hardware->createSyncObjects();

    return SeResult::SE_SUCCESS;
}

void SERuntine::updateAndRender()
{
    if (m_hardware)
    {
        m_hardware->updateAndRender();
    }
}

SeTextureHandle SERuntine::getViewportTex(uint32_t currentFrame) const
{
    if (!m_hardware)
        return VK_NULL_HANDLE;

    return m_hardware->getViewportTex(currentFrame);
}

}  // namespace SE