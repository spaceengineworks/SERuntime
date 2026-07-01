#include "SERuntime.hpp"

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

}  // namespace SE