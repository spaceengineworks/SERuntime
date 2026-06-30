#pragma once

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
class SERuntine
{
};
}  // namespace SE