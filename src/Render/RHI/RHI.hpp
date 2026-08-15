#ifndef RHI_H
#define RHI_H

#include <cstdint>
#include <vector>

#include "../../space_core.hpp"
#include "../FrameGraph/FrameGraph.hpp"
#include "../ThirdParty/thsvs_simpler_RHI.h"

namespace SE
{
class FrameGraph;

enum SeRenderType
{
    Vulkan
};

using SeRender = SeRenderType;

enum class ResourseType
{
    Buffer,
    Image
};

using SeRenderHandle   = void*;
using SeTextureHandle  = void*;
using SeResourseHandle = void*;

struct BarrierDesc
{
    SeResourseHandle resourse;
    ResourseType     type;

    std::vector<ThsvsAccessType> prevAccesses;
    std::vector<ThsvsAccessType> nextAccesses;

    ThsvsImageLayout prevLayout      = THSVS_IMAGE_LAYOUT_OPTIMAL;
    ThsvsImageLayout nextLayout      = THSVS_IMAGE_LAYOUT_OPTIMAL;
    bool             discardContents = false;

    uint32_t baseMipLevel   = 0;
    uint32_t levelCount     = 1;
    uint32_t baseArrayLayer = 0;
    uint32_t layerCount     = 1;

    uint32_t srcQueueFamily = ~0u;
    uint32_t dstQueueFamily = ~0u;

    uint64_t offset = 0;
    uint64_t size   = 0;
};

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

    virtual void             updateAndRender(FrameGraph* frameGraph)     = 0;
    virtual SeTextureHandle  getViewportTex(uint32_t frameIndex)         = 0;
    virtual SeResourseHandle getViewportImageHandle(uint32_t frameIndex) = 0;
    virtual const uint32_t*  getCurrentFrameIndex()                      = 0;

    virtual SeResult needVPResize(const uint32_t* width, const uint32_t* height) = 0;
    virtual void     resizeVP(const uint32_t* width, const uint32_t* height)     = 0;

    virtual uint32_t getFramesInFlightCount() const = 0;

    virtual void setClearColor(float r, float g, float b) = 0;

    /* TODO: temp */
    virtual void insertPipelineBarrier(BarrierDesc* barrier) = 0;

    virtual void beginRenderPass() = 0;

    /* frame graph methods */
    virtual void callDraw()             = 0;
    virtual void bindPipe(void* handle) = 0;
    virtual void setViewport()          = 0;
    virtual void setScissor()           = 0;
    virtual void endRenderPass()        = 0;
    /*---------------------*/
};
}  // namespace SE

#endif /* RHI_H */