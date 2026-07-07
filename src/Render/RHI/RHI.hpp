#ifndef RHI_H
#define RHI_H

#include <cstdint>

namespace SE
{
enum SeRenderType
{
    Vulkan
};

using SeRender = SeRenderType;

enum SeResultInternals
{
    SE_SUCCESS = 0,
    SE_FAILED  = 1,
    SE_RESIZED = 2
};

using SeResult        = SeResultInternals;
using SeRenderHandle  = void*;
using SeTextureHandle = void*;

class RHI
{
   public:
    virtual ~RHI()                      = default;
    virtual SeRenderHandle passConfig() = 0;

    virtual void CreateViewPortImage(uint32_t width, uint32_t height) = 0;
    virtual void recreateViewPort()                                   = 0;
    virtual void cleanViewPort()                                      = 0;
    virtual void createOffscreenFramebuffer()                         = 0;
    virtual void createOffscreenRenderPass()                          = 0;
    virtual void viewPortCommandBuffer()                              = 0;
    virtual void createSyncObjects()                                  = 0;
    virtual void cleanUp()                                            = 0;

    virtual void            updateAndRender()                   = 0;
    virtual SeTextureHandle getViewportTex(uint32_t frameIndex) = 0;
    virtual uint32_t        getCurrentFrameIndex()              = 0;

    virtual SeResult needVPResize(const uint32_t* width, const uint32_t* height) = 0;
    virtual void     resizeVP(const uint32_t* width, const uint32_t* height)     = 0;

    virtual uint32_t getFramesInFlightCount() const = 0;

    virtual void setClearColor(float r, float g, float b) = 0;
};
}  // namespace SE

#endif /* RHI_H */