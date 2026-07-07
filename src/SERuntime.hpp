#ifndef SERUNTIME_H
#define SERUNTIME_H

#include <cstdint>
#include <memory>

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

namespace SE
{

class SERUNTIME_API SERuntine
{
   public:
    SERuntine(SeRender render);
    ~SERuntine();

    SeRenderHandle  SeAskConfig();
    SeResult        initViewPort(uint32_t width, uint32_t height);
    void            updateAndRender();
    SeTextureHandle getViewportTex(uint32_t currentFrame) const;
    void            deviceCleanUp()
    {
        m_hardware->cleanUp();
    }

    void setViewportColor(float r, float g, float b)
    {
        if (m_hardware)
            m_hardware->setClearColor(r, g, b);
    }

    bool checkResizeViewPort(const uint32_t& width, const uint32_t& height)
    {
        if (m_hardware->needVPResize(&width, &height))
        {
            m_hardware->resizeVP(&width, &height);
            return true;
        }
        return false;
    }

    const uint32_t* getCurrentFrameIndex() const
    {
        return m_hardware ? m_hardware->getCurrentFrameIndex() : 0;
    }

    uint32_t getFramesInFlight() const
    {
        return m_hardware ? m_hardware->getFramesInFlightCount() : 0;
    }

   private:
    std::unique_ptr<RHI> m_hardware = nullptr;
};
}  // namespace SE

#endif /* SERUNTIME_H */