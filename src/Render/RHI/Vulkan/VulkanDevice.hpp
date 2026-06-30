#ifndef VULKANDEVICE_HPP

#include <vulkan/vulkan.h>

#include "RHI/RHI.hpp"

namespace SE
{
class VulkanDevice : public RHI
{
   public:
    SeResult initConfig(const RHI::Config& config) override
    {
        m_config = static_cast<const VulkanConfig&>(config);
        return (m_config.isValid()) ? SE_SUCCESS : SE_FAILED;
    }

   private:
    struct VulkanConfig : public RHI::Config
    {
        VkInstance       instance       = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice         device         = VK_NULL_HANDLE;
        VkSurfaceKHR     surface        = VK_NULL_HANDLE;

        VkQueue  graphicsQueue  = VK_NULL_HANDLE;
        VkQueue  presentQueue   = VK_NULL_HANDLE;
        uint32_t graphicsFamily = 0;
        uint32_t presentFamily  = 0;

        VkSwapchainKHR swapChain           = VK_NULL_HANDLE;
        VkFormat       swapChainFormat     = VK_FORMAT_UNDEFINED;
        VkExtent2D     swapChainExtent     = {0, 0};
        uint32_t       swapChainImageCount = 0;

        VkViewport       viewport         = {};
        VkRect2D         scissor          = {};
        VkRenderPass     renderPass       = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout   = VK_NULL_HANDLE;
        VkPipeline       graphicsPipeline = VK_NULL_HANDLE;
        VkCommandPool    commandPool      = VK_NULL_HANDLE;

        uint32_t maxFramesInFlight = 0;

        bool isValid()
        {
            return instance != VK_NULL_HANDLE && physicalDevice != VK_NULL_HANDLE
                   && device != VK_NULL_HANDLE && surface != VK_NULL_HANDLE
                   && graphicsQueue != VK_NULL_HANDLE && presentQueue != VK_NULL_HANDLE
                   && swapChain != VK_NULL_HANDLE && renderPass != VK_NULL_HANDLE
                   && pipelineLayout != VK_NULL_HANDLE && graphicsPipeline != VK_NULL_HANDLE
                   && commandPool != VK_NULL_HANDLE && swapChainFormat != VK_FORMAT_UNDEFINED
                   && swapChainExtent.width > 0 && swapChainExtent.height > 0
                   && swapChainImageCount > 0 && maxFramesInFlight > 0 && scissor.extent.width > 0
                   && scissor.extent.height > 0;
        }
    } m_config;
};
}  // namespace SE

#endif /* VULKANDEVICE_HPP */