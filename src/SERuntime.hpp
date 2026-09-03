#ifndef SERUNTIME_H
#define SERUNTIME_H

#include <cstdint>
#include <memory>

#include "Render/FrameGraph/FrameGraph.hpp"
#include "Render/Managers/BufferManager/IBufferManager.hpp"
#include "Render/Managers/DescriptorManager/IDescriptorManager.hpp"
#include "Render/Managers/PipelineManager/IPipelineManager.hpp"
#include "Render/Managers/ShaderManager/IShaderManager.hpp"
#include "Render/Managers/TextureManager/ITextureManager.hpp"
//
#include "Render/Managers/MeshCollection.hpp"
#include "Render/Managers/MeshCollectionFabric.hpp"

//

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

// TODO:
//
//  1. add to populate buffer switch to choose between async and sync.
//  2. build() FrameGraph.
//  3. add full G buffer pass.
//  4. clean all includes \ and ThirdParty files.
//  5. clean up dead/commented code in VulkanDevice destructor, decide cleanUp() vs dtor
//  6. need refactor to beginSingleTimeCommands and beginSingleTimeCommands use RAII ( just nest in one { //code  } ).
//  7. refactor cmake make all submodules dont be dependent from vcpkg.
//  8. add tracy through own macros make it for debug and release

// Important: some how make more stable creation engine and passing viewport to Editor to much boilercode.

// 0. ECS: add World.hpp/.cpp with EnTT registry, call update from runtime loop
// 1. Physics: add Jolt world init + step call from runtime loop

// Note:
// Buffer manager can add bugs with adding data to all buffers.

namespace SE
{
class SERUNTIME_API SERuntime
{
   public:
    SERuntime(SeRender render);
    ~SERuntime();

    SeRenderHandle  SeAskConfig();
    void            initEngine();
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
    SeRender                              m_API_render;
    std::unique_ptr<RHI>                  m_context    = nullptr;
    std::unique_ptr<FrameGraph>           m_frameGraph = nullptr;
    std::unique_ptr<MeshCollectionFabric> m_meshFabric = nullptr;
    FrameAllocator                        m_frameAllocator;

    std::unique_ptr<IShaderManager>     m_shaderManager;
    std::unique_ptr<ITextureManager>    m_textureManager;
    std::unique_ptr<IPipelineManager>   m_pipelineManager;
    std::unique_ptr<IDescriptorManager> m_descriptorManager;
    std::unique_ptr<IBufferManager>     m_bufferManager;
};
}  // namespace SE

#endif /* SERUNTIME_H */