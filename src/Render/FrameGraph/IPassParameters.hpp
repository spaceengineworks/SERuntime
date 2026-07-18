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
    int          handle;
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
        return {{ResourceType::none, -1}};
    }

   private:
};

};  // namespace SE

#endif /* IPASS_PARAMETERS_H */