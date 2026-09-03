#ifndef FRAME_PASS_H
#define FRAME_PASS_H

#include <functional>
#include <ranges>

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
    FramePass(std::string name, GPUFlags flags, std::vector<ResourceField> resourceHandle, std::function<void(RHI*)> callback)
        : m_passName(std::move(name)), m_passflags(flags), m_passParameters(std::move(resourceHandle)), m_passCallback(std::move(callback))
    {
    }
    void execute(RHI* ctx) const
    {
        if (m_passCallback)
            m_passCallback(ctx);
    }

    // clang-format off

    auto reads() const
    {
        return m_passParameters | std::views::filter(
            [](const ResourceField& f) { return f.access == ResourceAccess::Read; });
    }

    auto writes() const
    {
        return m_passParameters | std::views::filter(
            [](const ResourceField& f) { return f.access == ResourceAccess::Write; });
    }

    // clang-format on

    std::string                m_passName;
    GPUFlags                   m_passflags;
    std::vector<ResourceField> m_passParameters;
    std::function<void(RHI*)>  m_passCallback;
};

}  // namespace SE

#endif /* FRAME_PASS_H */