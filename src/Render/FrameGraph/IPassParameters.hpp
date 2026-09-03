#ifndef IPASS_PARAMETERS_H
#define IPASS_PARAMETERS_H

#include <vector>

#include "../ThirdParty/thsvs_simpler_RHI.h"

namespace SE
{

constexpr uint32_t VIEWPORT_DEFAULT_HANDLE = UINT32_MAX;

enum class ResourceType
{
    none,
    shader,
    texture,
    index,
    vertex
};

enum class ResourceAccess : uint8_t
{
    Read,
    Write
};

struct ResourceField
{
    ResourceType    type   = ResourceType::none;
    ResourceAccess  access = ResourceAccess::Read;
    uint32_t        handle;
    ThsvsAccessType gpuAccess      = THSVS_ACCESS_NONE;
    bool            isRenderTarget = false;
};

inline ThsvsAccessType inferDefaultGpuAccess(ResourceType type, ResourceAccess access)
{
    switch (type)
    {
        case ResourceType::texture:
            return access == ResourceAccess::Write ? THSVS_ACCESS_COLOR_ATTACHMENT_WRITE : THSVS_ACCESS_FRAGMENT_SHADER_READ_SAMPLED_IMAGE_OR_UNIFORM_TEXEL_BUFFER;
        case ResourceType::vertex:
            return THSVS_ACCESS_VERTEX_BUFFER;
        case ResourceType::index:
            return THSVS_ACCESS_INDEX_BUFFER;
        case ResourceType::shader:
        default:
            return THSVS_ACCESS_GENERAL;
    }
}

class IPassParameters
{
   public:
    virtual ~IPassParameters() = default;
    virtual std::vector<ResourceField> getResources()
    {
        return {};
    }

   private:
};

class PassClearParameters : public IPassParameters
{
   public:
    PassClearParameters()           = default;
    ~PassClearParameters() override = default;

    float m_clearColor[4];

    std::vector<ResourceField> getResources() override
    {
        return {{ResourceType::texture, ResourceAccess::Write, VIEWPORT_DEFAULT_HANDLE, THSVS_ACCESS_NONE, true}};
    }

   private:
};

class PassDrawParameters : public IPassParameters
{
   public:
    PassDrawParameters()           = default;
    ~PassDrawParameters() override = default;

    uint32_t textureHandle    = 0;
    uint32_t descHandle       = 0;
    uint32_t collectionHandle = 0;

    float mvp[16] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};

    std::vector<ResourceField> getResources() override
    {
        return {
            {ResourceType::texture, ResourceAccess::Read, textureHandle, THSVS_ACCESS_NONE, false},
            {ResourceType::texture, ResourceAccess::Write, VIEWPORT_DEFAULT_HANDLE, THSVS_ACCESS_NONE, true},
        };
    }

   private:
};

class PassGBufferParameters : public IPassParameters
{
   public:
    PassGBufferParameters()           = default;
    ~PassGBufferParameters() override = default;

    std::vector<ResourceField> getResources() override
    {
        return {

        };
    }

   private:
};

};  // namespace SE

#endif /* IPASS_PARAMETERS_H */