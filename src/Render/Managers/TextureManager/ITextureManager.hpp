#ifndef ITEXTURE_MANAGER_H
#define ITEXTURE_MANAGER_H

#include <cstdint>

#include "../../../space_core.hpp"

namespace SE
{

using SeTextureID     = uint32_t;
using SeTextureHandle = void*;

struct TextureDesc
{
    uint32_t imageType;
    uint32_t format;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t mipLevels;
    uint32_t levelCount;
    uint32_t arrayLayers;
    uint32_t layerCount;
    uint32_t sampleCount;
    uint32_t tiling;
    uint32_t usage;
    uint32_t sharingMode;
    uint32_t initialLayout;

    const void* pixelData;
    size_t      dataSize;
};

class ITextureManager
{
   public:
    virtual ~ITextureManager() = default;

    virtual SeTextureID     createTexture(TextureDesc desc)         = 0;
    virtual SeResult        destroyTexture(SeTextureID textureId)   = 0;
    virtual SeTextureHandle getTextureHandle(SeTextureID textureId) = 0;

   private:
};

}  // namespace SE

#endif /* ITEXTURE_MANAGER_H */