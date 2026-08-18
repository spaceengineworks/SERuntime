#define VMA_IMPLEMENTATION
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#include "RHI/Vulkan/VulkanDevice.hpp"

#include <iostream>
#include <tracy/Tracy.hpp>

#define THSVS_SIMPLER_VULKAN_SYNCHRONIZATION_IMPLEMENTATION
#include "../../ThirdParty/thsvs_simpler_vulkan_synchronization.h"

namespace SE

{
VulkanDevice::VulkanDevice()
{
    m_config = std::make_unique<SharedVulkanConfig>();
}

VulkanDevice::~VulkanDevice()
{
    // if (m_config && *m_config->device != VK_NULL_HANDLE)
    // {
    //     VkDevice device = *m_config->device;

    //     if (!m_commandBuffers.empty() && *m_config->commandPool != VK_NULL_HANDLE)
    //     {
    //         vkFreeCommandBuffers(device, *m_config->commandPool,
    //                              static_cast<uint32_t>(m_commandBuffers.size()),
    //                              m_commandBuffers.data());
    //     }

    //     for (auto framebuffer : m_viewPortFramebuffers)
    //     {
    //         if (framebuffer != VK_NULL_HANDLE)
    //         {
    //             vkDestroyFramebuffer(device, framebuffer, nullptr);
    //         }
    //     }

    //     if (m_renderPass != VK_NULL_HANDLE)
    //     {
    //         vkDestroyRenderPass(device, m_renderPass, nullptr);
    //     }

    //     for (auto& vp : m_viewPort)
    //     {
    //         // if (vp.imageView != VK_NULL_HANDLE)
    //         //     vkDestroyImageView(device, vp.imageView, nullptr);
    //         // if (vp.image != VK_NULL_HANDLE)
    //         //     vkDestroyImage(device, vp.image, nullptr);
    //         // if (vp.imageMemory != VK_NULL_HANDLE)
    //         //     vkFreeMemory(device, vp.imageMemory, nullptr);
    //     }
    // }
}

void VulkanDevice::CreateViewPortImage(uint32_t width, uint32_t height)
{
    m_viewPortWidth  = width;
    m_viewPortHeight = height;

    m_sampler = getOrCreateDefaultSampler();

    if (m_viewPort.empty())
    {
        m_viewPort.resize(*m_config->frame_in_flight);
    }

    VkDevice device = *m_config->device;

    VkCommandBufferAllocateInfo allocInfoCmd {};
    allocInfoCmd.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfoCmd.commandPool        = *m_config->commandPool;
    allocInfoCmd.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfoCmd.commandBufferCount = 1;

    VkCommandBuffer transitionCmd;
    vkAllocateCommandBuffers(device, &allocInfoCmd, &transitionCmd);

    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(transitionCmd, &beginInfo);

    for (uint32_t i = 0; i < *m_config->frame_in_flight; i++)
    {
        VkImageCreateInfo imageInfo {};
        imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType     = VK_IMAGE_TYPE_2D;
        imageInfo.extent        = {width, height, 1};
        imageInfo.mipLevels     = 1;
        imageInfo.arrayLayers   = 1;
        imageInfo.format        = VK_FORMAT_R8G8B8A8_UNORM;
        imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocCreateInfo {};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        vmaCreateImage(*m_config->allocator, &imageInfo, &allocCreateInfo, &m_viewPort[i].image, &m_viewPort[i].imageAllocation, nullptr);

        VkImageCreateInfo depthInfo {};
        depthInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        depthInfo.imageType     = VK_IMAGE_TYPE_2D;
        depthInfo.extent        = {width, height, 1};
        depthInfo.mipLevels     = 1;
        depthInfo.arrayLayers   = 1;
        depthInfo.format        = VK_FORMAT_D32_SFLOAT;
        depthInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
        depthInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthInfo.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        depthInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
        depthInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo depthAllocInfo {};
        depthAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        vmaCreateImage(*m_config->allocator, &depthInfo, &depthAllocInfo, &m_viewPort[i].depthImage, &m_viewPort[i].depthImageAllocation, nullptr);

        VkImageViewCreateInfo depthViewInfo {};
        depthViewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        depthViewInfo.image                           = m_viewPort[i].depthImage;
        depthViewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        depthViewInfo.format                          = VK_FORMAT_D32_SFLOAT;
        depthViewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
        depthViewInfo.subresourceRange.baseMipLevel   = 0;
        depthViewInfo.subresourceRange.levelCount     = 1;
        depthViewInfo.subresourceRange.baseArrayLayer = 0;
        depthViewInfo.subresourceRange.layerCount     = 1;

        vkCreateImageView(device, &depthViewInfo, nullptr, &m_viewPort[i].depthImageView);

        VkImageViewCreateInfo createInfo {};
        createInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image                           = m_viewPort[i].image;
        createInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format                          = VK_FORMAT_R8G8B8A8_UNORM;
        createInfo.components                      = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
        createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel   = 0;
        createInfo.subresourceRange.levelCount     = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount     = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &m_viewPort[i].imageView) != VK_SUCCESS)
            throw std::runtime_error("failed to create viewport image view!");

        VkImageMemoryBarrier barrier {};
        barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                           = m_viewPort[i].image;
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;
        barrier.srcAccessMask                   = 0;
        barrier.dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(transitionCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    vkEndCommandBuffer(transitionCmd);

    VkSubmitInfo submitInfo {};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &transitionCmd;

    vkQueueSubmit(*m_config->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(*m_config->graphicsQueue);

    vkFreeCommandBuffers(device, *m_config->commandPool, 1, &transitionCmd);
}

void VulkanDevice::viewPortCommandBuffer()
{
    uint32_t framesInFlight = *m_config->frame_in_flight;
    m_commandBuffers.resize(framesInFlight);

    VkCommandBufferAllocateInfo memAllocInfo {};
    memAllocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    memAllocInfo.commandPool        = *m_config->commandPool;
    memAllocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    memAllocInfo.commandBufferCount = framesInFlight;

    if (vkAllocateCommandBuffers(*m_config->device, &memAllocInfo, m_commandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate offscreen command buffers!");
    }
}

uint32_t VulkanDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(*m_config->physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;

    throw std::runtime_error("failed to find suitable memory type!");
}

void VulkanDevice::createOffscreenFramebuffer()
{
    m_viewPortFramebuffers.resize(m_viewPort.size());

    for (size_t i = 0; i < m_viewPort.size(); i++)
    {
        VkImageView attachments[] = {m_viewPort[i].imageView, m_viewPort[i].depthImageView};

        VkFramebufferCreateInfo framebufferInfo {};
        framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass      = m_renderPass;
        framebufferInfo.attachmentCount = 2;
        framebufferInfo.pAttachments    = attachments;
        framebufferInfo.width           = m_viewPortWidth;
        framebufferInfo.height          = m_viewPortHeight;
        framebufferInfo.layers          = 1;

        if (vkCreateFramebuffer(*m_config->device, &framebufferInfo, nullptr, &m_viewPortFramebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void VulkanDevice::createOffscreenRenderPass()
{
    VkAttachmentDescription colorAttachment {};
    colorAttachment.format  = VK_FORMAT_R8G8B8A8_UNORM;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

    colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference colorAttachmentRef {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment {};
    depthAttachment.format         = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass {};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    std::array<VkSubpassDependency, 2> dependencies {};

    dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass      = 0;
    dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask   = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass      = 0;
    dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].dstAccessMask   = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

    VkRenderPassCreateInfo renderPassInfo {};

    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments    = attachments.data();
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies   = dependencies.data();

    if (vkCreateRenderPass(*m_config->device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    }
}

//  -------------------- //

void VulkanDevice::recreateViewPort()
{
    vkDeviceWaitIdle(*m_config->device);

    cleanViewPort();
    CreateViewPortImage(m_viewPortWidth, m_viewPortHeight);
    createOffscreenFramebuffer();
}

void VulkanDevice::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, FrameGraph* frameGraph)
{
    ZoneScopedN("VulkanDevice::recordCommandBuffer");

    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    // VkRenderPassBeginInfo renderPassInfo {};
    // renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    // renderPassInfo.renderPass        = m_renderPass;
    // renderPassInfo.framebuffer       = m_viewPortFramebuffers[imageIndex];
    // renderPassInfo.renderArea.offset = {0, 0};

    // renderPassInfo.renderArea.extent = {m_viewPortWidth, m_viewPortHeight};

    // VkClearValue clearColor        = {{{m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3]}}};
    // renderPassInfo.clearValueCount = 1;
    // renderPassInfo.pClearValues    = &clearColor;

    // vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
    // *m_config->graphicsPipeline);

    if (vkBeginCommandBuffer(m_currCmdBuff, &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    frameGraph->execute();

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void VulkanDevice::updateAndRender(FrameGraph* frameGraph)
{
    ZoneScopedN("VulkanDevice::updateAndRender");
    if (m_viewPortWidth <= 0 || m_viewPortHeight <= 0)
        return;

    if (m_VPResized)
    {
        m_VPResized = false;
        recreateViewPort();
    }

    uint32_t currentFrame = *m_config->currentFrame;

    {
        ZoneScopedN("vkWaitForFences (SE)");
        vkWaitForFences(*m_config->device, 1, &m_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
        vkResetFences(*m_config->device, 1, &m_inFlightFences[currentFrame]);
    }

    // VkCommandBuffer cmdBuf = m_commandBuffers[currentFrame];
    // vkResetCommandBuffer(cmdBuf, 0);
    // recordCommandBuffer(cmdBuf, currentFrame);

    // VkSubmitInfo submitInfo {};
    // submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    // submitInfo.commandBufferCount = 1;
    // submitInfo.pCommandBuffers    = &cmdBuf;

    m_currCmdBuff = m_commandBuffers[currentFrame];
    vkResetCommandBuffer(m_currCmdBuff, 0);
    recordCommandBuffer(m_currCmdBuff, currentFrame, frameGraph);

    VkSubmitInfo submitInfo {};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &m_currCmdBuff;

    vkQueueSubmit(*m_config->graphicsQueue, 1, &submitInfo, m_inFlightFences[currentFrame]);
}

void VulkanDevice::createSyncObjects()
{
    uint32_t framesInFlight = *m_config->frame_in_flight;
    m_inFlightFences.resize(framesInFlight);

    VkFenceCreateInfo fenceInfo {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (auto& fence : m_inFlightFences) vkCreateFence(*m_config->device, &fenceInfo, nullptr, &fence);
}

void VulkanDevice::cleanViewPort()
{
    VkDevice device = *m_config->device;

    for (auto framebuffer : m_viewPortFramebuffers)
    {
        if (framebuffer != VK_NULL_HANDLE)
            vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    m_viewPortFramebuffers.clear();

    for (auto viewPort : m_viewPort)
    {
        vkDestroyImageView(device, viewPort.imageView, nullptr);
        vmaDestroyImage(*m_config->allocator, viewPort.image, viewPort.imageAllocation);

        if (viewPort.depthImageView != VK_NULL_HANDLE)
            vkDestroyImageView(device, viewPort.depthImageView, nullptr);
        if (viewPort.depthImage != VK_NULL_HANDLE)
            vmaDestroyImage(*m_config->allocator, viewPort.depthImage, viewPort.depthImageAllocation);
    }
    m_viewPort.clear();
}

void VulkanDevice::cleanUp()
{
    vkDeviceWaitIdle(*m_config->device);

    if (m_config && *m_config->device != VK_NULL_HANDLE)
    {
        VkDevice device = *m_config->device;

        cleanViewPort();

        if (m_sampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(device, m_sampler, nullptr);
            m_sampler = VK_NULL_HANDLE;
        }
        for (auto& fence : m_inFlightFences)
        {
            if (fence != VK_NULL_HANDLE)
                vkDestroyFence(device, fence, nullptr);
        }
        m_inFlightFences.clear();

        if (!m_commandBuffers.empty() && *m_config->commandPool != VK_NULL_HANDLE)
        {
            vkFreeCommandBuffers(device, *m_config->commandPool, static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
            m_commandBuffers.clear();
        }

        for (auto framebuffer : m_viewPortFramebuffers)
        {
            if (framebuffer != VK_NULL_HANDLE)
                vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
        m_viewPortFramebuffers.clear();

        if (m_renderPass != VK_NULL_HANDLE)
        {
            vkDestroyRenderPass(device, m_renderPass, nullptr);
            m_renderPass = VK_NULL_HANDLE;
        }
    }
}

SeTextureHandle VulkanDevice::getViewportTex(uint32_t frameIndex)
{
    if (frameIndex >= *m_config->frame_in_flight)
        return VK_NULL_HANDLE;

    return static_cast<SeTextureHandle>(&m_viewPort[frameIndex]);
}

SeResourseHandle VulkanDevice::getViewportImageHandle(uint32_t frameIndex)
{
    if (frameIndex >= *m_config->frame_in_flight)
        return nullptr;

    return reinterpret_cast<SeResourseHandle>(m_viewPort[frameIndex].image);
}

const uint32_t* VulkanDevice::getCurrentFrameIndex()
{
    return m_config->currentFrame;
}

VkSampler VulkanDevice::getOrCreateDefaultSampler()
{
    if (m_sampler != VK_NULL_HANDLE)
        return m_sampler;

    VkSamplerCreateInfo samplerInfo {};
    samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter               = VK_FILTER_LINEAR;
    samplerInfo.minFilter               = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.mipLodBias              = 0.0f;
    samplerInfo.anisotropyEnable        = VK_FALSE;
    samplerInfo.maxAnisotropy           = 1.0f;
    samplerInfo.compareEnable           = VK_FALSE;
    samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
    samplerInfo.minLod                  = 0.0f;
    samplerInfo.maxLod                  = 0.0f;
    samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    if (vkCreateSampler(*m_config->device, &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create default sampler!");
    }

    return m_sampler;
}

/* TODO: temp */

void VulkanDevice::insertPipelineBarrier(BarrierDesc* barrier)
{
    if (barrier->type == ResourseType::Image)
    {
        ThsvsImageBarrier imageBarrier {};

        imageBarrier.prevAccessCount = static_cast<uint32_t>(barrier->prevAccesses.size());
        imageBarrier.pPrevAccesses   = barrier->prevAccesses.data();
        imageBarrier.nextAccessCount = static_cast<uint32_t>(barrier->nextAccesses.size());
        imageBarrier.pNextAccesses   = barrier->nextAccesses.data();

        imageBarrier.prevLayout      = barrier->prevLayout;
        imageBarrier.nextLayout      = barrier->nextLayout;
        imageBarrier.discardContents = barrier->discardContents ? VK_TRUE : VK_FALSE;

        imageBarrier.srcQueueFamilyIndex = barrier->srcQueueFamily;
        imageBarrier.dstQueueFamilyIndex = barrier->dstQueueFamily;

        imageBarrier.image            = reinterpret_cast<VkImage>(barrier->resourse);
        imageBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, barrier->baseMipLevel, barrier->levelCount, barrier->baseArrayLayer, barrier->layerCount};

        thsvsCmdPipelineBarrier(m_currCmdBuff, nullptr, 0, nullptr, 1, &imageBarrier);
    }
    else if (barrier->type == ResourseType::Buffer)
    {
        ThsvsBufferBarrier bufferBarrier {};

        bufferBarrier.prevAccessCount = static_cast<uint32_t>(barrier->prevAccesses.size());
        bufferBarrier.pPrevAccesses   = barrier->prevAccesses.data();
        bufferBarrier.nextAccessCount = static_cast<uint32_t>(barrier->nextAccesses.size());
        bufferBarrier.pNextAccesses   = barrier->nextAccesses.data();

        bufferBarrier.srcQueueFamilyIndex = barrier->srcQueueFamily;
        bufferBarrier.dstQueueFamilyIndex = barrier->dstQueueFamily;

        bufferBarrier.buffer = reinterpret_cast<VkBuffer>(barrier->resourse);
        bufferBarrier.offset = barrier->offset;
        bufferBarrier.size   = barrier->size;

        thsvsCmdPipelineBarrier(m_currCmdBuff, nullptr, 1, &bufferBarrier, 0, nullptr);
    }
}

void VulkanDevice::beginRenderPass()
{
    VkRenderPassBeginInfo renderPassInfo {};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = m_renderPass;
    renderPassInfo.framebuffer       = m_viewPortFramebuffers[*m_config->currentFrame];
    renderPassInfo.renderArea.offset = {0, 0};

    renderPassInfo.renderArea.extent = {m_viewPortWidth, m_viewPortHeight};

    std::array<VkClearValue, 2> clearValues {};
    clearValues[0].color        = {{m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3]}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues    = clearValues.data();

    vkCmdBeginRenderPass(m_currCmdBuff, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

}  // namespace SE