#ifndef SERUNTIME_H
#define SERUNTIME_H

#include <cstdint>
#include <memory>

#include "Render/FrameGraph/FrameGraph.hpp"
#include "Render/RHI/RHI.hpp"

#if defined(_WIN32) || defined(_WIN64)

#ifdef SERUNTIME_EXPORTS
#define SERUNTIME_API __declspec(dllexport)
#else
#define SERUNTIME_API __declspec(dllimport)
#endif

#else

#ifdef SERUNTIME_EXPORTS
#define SERUNTIME_API __attribute__((visibility("default")))
#else
#define SERUNTIME_API
#endif

#endif

// 0. add normal namespace inside other namespace
// 1. Add a second pass (geometry pass) using shaderc pipeline + vertex/index buffer to test the graph
// 2. ECS: add World.hpp/.cpp with EnTT registry, call update from runtime loop
// 3. Physics: add Jolt world init + step call from runtime loop
// 4. Clean up dead/commented code in VulkanDevice destructor, decide cleanUp() vs dtor

namespace SE
{
class SERUNTIME_API SERuntime
{
   public:
    SERuntime(SeRender render);
    ~SERuntime();

    SeRenderHandle  SeAskConfig();
    SeResult        initViewPort(uint32_t width, uint32_t height);
    void            updateAndRender();
    SeTextureHandle getViewportTex(uint32_t currentFrame) const;
    void            deviceCleanUp()
    {
        m_context->cleanUp();
    }

    void setViewportColor(float r, float g, float b)
    {
        if (m_context)
            m_context->setClearColor(r, g, b);
    }

    bool checkResizeViewPort(const uint32_t& width, const uint32_t& height)
    {
        if (m_context->needVPResize(&width, &height))
        {
            m_context->resizeVP(&width, &height);
            return true;
        }
        return false;
    }

    const uint32_t* getCurrentFrameIndex() const
    {
        return m_context ? m_context->getCurrentFrameIndex() : 0;
    }

    uint32_t getFramesInFlight() const
    {
        return m_context ? m_context->getFramesInFlightCount() : 0;
    }

   private:
    std::unique_ptr<RHI>        m_context    = nullptr;
    std::unique_ptr<FrameGraph> m_frameGraph = nullptr;
    FrameAllocator              m_frameAllocator;
};
}  // namespace SE

#endif /* SERUNTIME_H */