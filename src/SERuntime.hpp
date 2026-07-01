#ifndef SERUNTIME_H
#define SERUNTIME_H

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

    SeRenderHandle SeAskConfig();

   private:
    std::unique_ptr<RHI> m_hardware = nullptr;
};
}  // namespace SE

#endif /* SERUNTIME_H */