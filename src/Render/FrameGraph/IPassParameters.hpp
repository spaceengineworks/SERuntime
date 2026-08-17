#ifndef IPASS_PARAMETERS_H
#define IPASS_PARAMETERS_H

#include <vector>

namespace SE
{

enum class ResourceType
{
    none,
    shader,
    texture,
    index,
    vertex
};

struct ResourceField
{
    ResourceType type = ResourceType::none;
    uint32_t     handle;
};

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
        return {{ResourceType::none, UINT32_MAX}};
    }

   private:
};

class PassDrawParameters : public IPassParameters
{
   public:
    PassDrawParameters()           = default;
    ~PassDrawParameters() override = default;

    uint32_t textureHandle = 0;
    uint32_t descHandle    = 0;

    std::vector<ResourceField> getResources() override
    {
        return {{ResourceType::texture, textureHandle}};
    }

   private:
};

};  // namespace SE

#endif /* IPASS_PARAMETERS_H */