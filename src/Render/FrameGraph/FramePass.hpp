#ifndef FRAME_PASS_H
#define FRAME_PASS_H

#include <functional>

#include "IPassParameters.hpp"

namespace SE
{
class RHI;

enum class GPUFlags
{
    Undefined,
    Compute,
    Render
};

class FramePass
{
   public:
    FramePass(std::string_view name, GPUFlags flags, std::vector<ResourceField> resourceHandle, std::function<void(RHI*)> callback)
        : m_passName(name), m_passflags(flags), m_passParameters(std::move(resourceHandle)), m_passCallback(std::move(callback))
    {
    }
    void execute(RHI* ctx) const
    {
        if (m_passCallback)
            m_passCallback(ctx);
    }

    std::string_view           m_passName;
    GPUFlags                   m_passflags;
    std::vector<ResourceField> m_passParameters;
    std::function<void(RHI*)>  m_passCallback;
};

}  // namespace SE

#endif /* FRAME_PASS_H */