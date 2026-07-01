#ifndef VULKANDEVICE_HPP

#include <vulkan/vulkan.h>

#include <memory>
#include <string>

#include "RHI/RHI.hpp"
#include "VulkanConfig.hpp"

/*
vkCmdPipelineBarrier()
vkCmdBeginRenderPass()
vkCmdEndRenderPass()
https://docs.vulkan.org/refpages/latest/refpages/source/vkCmdPipelineBarrier.html
*/

namespace SE
{
class VulkanDevice : public RHI
{
   public:
    VulkanDevice();

    SeRenderHandle passConfig() override
    {
        return static_cast<SeRenderHandle>(m_config.get());
    }

   private:
    std::unique_ptr<SharedVulkanConfig> m_config = nullptr;
};
}  // namespace SE

#endif /* VULKANDEVICE_HPP */