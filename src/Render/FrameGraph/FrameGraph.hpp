#ifndef FRAME_GRAPH_H
#define FRAME_GRAPH_H

#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "../RHI/RHI.hpp"
#include "FrameAllocator.hpp"
#include "FramePass.hpp"
#include "IPassParameters.hpp"

namespace SE
{
class RHI;

class FrameGraph
{
   public:
    FrameGraph(RHI* context);
    ~FrameGraph() = default;

    void execute()
    {
        for (auto& pass : m_passes)
        {
            pass.execute(m_context);
        }
    }

    // TODO: build
    void build()
    {
        for (auto& pass : m_passes)
        {
            std::cout << pass.m_passName << std::endl;
        }
    }

    template <typename T>
    void add(std::string passName, GPUFlags passflags, T* passParameters, std::function<void(RHI*)> passCallback)
    {
        auto resources = passParameters->getResources();
        m_passes.emplace_back(std::move(passName), passflags, std::move(resources), std::move(passCallback));
    }

   private:
    std::vector<FramePass> m_passes;
    RHI*                   m_context = nullptr;
};

}  // namespace SE

#endif /* FRAME_GRAPH_H */
