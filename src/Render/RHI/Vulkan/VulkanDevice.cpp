#include "RHI/Vulkan/VulkanDevice.hpp"

namespace SE
{

VulkanDevice::VulkanDevice()
{
    m_config = std::make_unique<SharedVulkanConfig>();
}

}  // namespace SE