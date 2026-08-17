#include "VkTextureManager.hpp"

namespace SE::Render::Texture
{

VkTextureManager::VkTextureManager()
{
}

VkTextureManager::~VkTextureManager()
{
    for (Texture& tex : m_textures)
    {
        if (tex.data.imageView != VK_NULL_HANDLE)
            vkDestroyImageView(*m_config->device, tex.data.imageView, nullptr);

        if (tex.data.image != VK_NULL_HANDLE)
            vmaDestroyImage(*m_config->allocator, tex.data.image, tex.allocation);
    }

    for (VkSampler sampler : m_textureSamplers)
    {
        if (sampler != VK_NULL_HANDLE)
            vkDestroySampler(*m_config->device, sampler, nullptr);
    }
}

SeTextureID VkTextureManager::createTexture(TextureDesc desc)
{
    VkImageCreateInfo imageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};

    imageInfo.imageType     = static_cast<VkImageType>(desc.imageType);
    imageInfo.format        = static_cast<VkFormat>(desc.format);
    imageInfo.extent.width  = desc.width;
    imageInfo.extent.height = desc.height;
    imageInfo.extent.depth  = desc.depth;
    imageInfo.mipLevels     = desc.mipLevels;
    imageInfo.arrayLayers   = desc.arrayLayers;
    imageInfo.samples       = static_cast<VkSampleCountFlagBits>(desc.sampleCount);
    imageInfo.tiling        = static_cast<VkImageTiling>(desc.tiling);
    imageInfo.usage         = static_cast<VkImageUsageFlags>(desc.usage);
    imageInfo.sharingMode   = static_cast<VkSharingMode>(desc.sharingMode);
    imageInfo.initialLayout = static_cast<VkImageLayout>(desc.initialLayout);

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = VMA_MEMORY_USAGE_AUTO;

    VkImage       textureImage;
    VmaAllocation textureAllocation;

    if (vmaCreateImage(*(m_config->allocator), &imageInfo, &allocInfo, &textureImage, &textureAllocation, nullptr) != VK_SUCCESS)
    {
        return SE_INVALID_TEXTURE_ID;
    }

    VkImageView textureImageView;

    VkImageViewCreateInfo viewInfo {};
    viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                           = textureImage;
    viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                          = static_cast<VkFormat>(desc.format);
    viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = desc.levelCount;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = desc.layerCount;

    if (vkCreateImageView(*(m_config->device), &viewInfo, nullptr, &textureImageView) != VK_SUCCESS)
    {
        vmaDestroyImage(*(m_config->allocator), textureImage, textureAllocation);
        return SE_INVALID_TEXTURE_ID;
    }

    if (loadTexture(textureImage, &desc) != SE_SUCCESS)
    {
        vkDestroyImageView(*(m_config->device), textureImageView, nullptr);
        vmaDestroyImage(*(m_config->allocator), textureImage, textureAllocation);
        return SE_INVALID_TEXTURE_ID;
    }

    Texture texture;
    texture.data.image     = textureImage;
    texture.data.imageView = textureImageView;
    texture.data.sampler   = getOrCreateDefaultSampler(LINEAR);
    texture.allocation     = textureAllocation;

    if (!m_freeTextureIds.empty())
    {
        SeTextureID textureId = m_freeTextureIds.top();
        m_freeTextureIds.pop();

        m_textures[textureId] = texture;
        return textureId;
    }
    m_textures.push_back(texture);

    return m_nextId++;
}

SeResult VkTextureManager::destroyTexture(SeTextureID textureId)
{
    if (textureId == SE_INVALID_TEXTURE_ID || textureId >= m_textures.size())
        return FAILED_TO_DESTROY;

    Texture& tex = m_textures[textureId];

    if (tex.data.image == VK_NULL_HANDLE)
        return FAILED_TO_DESTROY;

    if (tex.data.imageView != VK_NULL_HANDLE)
        vkDestroyImageView(*(m_config->device), tex.data.imageView, nullptr);

    vmaDestroyImage(*(m_config->allocator), tex.data.image, tex.allocation);

    tex.data.image     = VK_NULL_HANDLE;
    tex.data.imageView = VK_NULL_HANDLE;
    tex.allocation     = nullptr;

    m_freeTextureIds.push(textureId);

    return SE_SUCCESS;
}

SeTextureHandle VkTextureManager::getTextureHandle(SeTextureID textureId)
{
    if (textureId == SE_INVALID_TEXTURE_ID || textureId >= m_textures.size())
        return nullptr;

    Texture& tex = m_textures[textureId];
    if (tex.data.image == VK_NULL_HANDLE)
        return nullptr;

    return static_cast<SeTextureHandle>(&tex.data);
}

/* Methods to change layout one time */

SeResult VkTextureManager::loadTexture(VkImage image, TextureDesc* desc)
{
    const VkDeviceSize pixelCount = static_cast<VkDeviceSize>(desc->width) * desc->height;
    if (pixelCount == 0 || desc->pixelData == nullptr || desc->dataSize == 0)
        return SE_FAILED;

    VkFormat     imageFormat      = VK_FORMAT_R8G8B8A8_UNORM;
    VkDeviceSize imageSize        = pixelCount * 4;
    const size_t expectedRgbaSize = static_cast<size_t>(pixelCount) * 4;
    const size_t expectedR8Size   = static_cast<size_t>(pixelCount);

    if (desc->dataSize == expectedR8Size)
    {
        imageFormat = VK_FORMAT_R8_UNORM;
        imageSize   = pixelCount;
    }
    else if (desc->dataSize != expectedRgbaSize)
        return SE_FAILED;

    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.usage              = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.size               = imageSize;
    bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfoCreate = {};
    allocInfoCreate.usage                   = VMA_MEMORY_USAGE_AUTO;
    allocInfoCreate.flags                   = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBuffer          stagingBuffer;
    VmaAllocation     stagingAllocation;
    VmaAllocationInfo stagingAllocInfo {};

    if (vmaCreateBuffer(*(m_config->allocator), &bufferInfo, &allocInfoCreate, &stagingBuffer, &stagingAllocation, &stagingAllocInfo) != VK_SUCCESS)
        return SE_FAILED;

    memcpy(stagingAllocInfo.pMappedData, desc->pixelData, static_cast<size_t>(imageSize));

    transitionImageLayout(image, imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer, image, desc);
    transitionImageLayout(image, imageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vmaDestroyBuffer(*(m_config->allocator), stagingBuffer, stagingAllocation);

    return SE_SUCCESS;
}

void VkTextureManager::copyBufferToImage(VkBuffer buffer, VkImage image, TextureDesc* desc)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region {};
    region.bufferOffset                    = 0;
    region.bufferRowLength                 = 0;
    region.bufferImageHeight               = 0;
    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;
    region.imageOffset                     = {0, 0, 0};
    region.imageExtent                     = {desc->width, desc->height, 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(commandBuffer);
}

VkCommandBuffer VkTextureManager::beginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool        = *(m_config->commandPool);
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(*(m_config->device), &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VkTextureManager::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo {};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer;

    vkQueueSubmit(*(m_config->graphicsQueue), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(*(m_config->graphicsQueue));

    vkFreeCommandBuffers(*(m_config->device), *(m_config->commandPool), 1, &commandBuffer);
}

void VkTextureManager::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier {};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = oldLayout;
    barrier.newLayout                       = newLayout;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = image;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    endSingleTimeCommands(commandBuffer);
}

VkSampler VkTextureManager::getOrCreateDefaultSampler(SamplerType type)
{
    if (m_textureSamplers[type] != VK_NULL_HANDLE)
        return m_textureSamplers[type];

    VkSamplerCreateInfo samplerInfo {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    switch (type)
    {
        case LINEAR:
            samplerInfo.magFilter  = VK_FILTER_LINEAR;
            samplerInfo.minFilter  = VK_FILTER_LINEAR;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

            samplerInfo.mipLodBias       = 0.0f;
            samplerInfo.anisotropyEnable = VK_FALSE;
            samplerInfo.maxAnisotropy    = 1.0f;

            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp     = VK_COMPARE_OP_ALWAYS;

            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

            samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            break;

        default:
            throw std::runtime_error("Unknown sampler type.");
    }

    if (vkCreateSampler(*m_config->device, &samplerInfo, nullptr, &m_textureSamplers[type]) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create sampler.");
    }

    return m_textureSamplers[type];
}

}  // namespace SE::Render::Texture