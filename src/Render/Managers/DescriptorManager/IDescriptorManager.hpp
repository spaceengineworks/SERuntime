#ifndef IDESCRIPTOR_MANAGER_HPP
#define IDESCRIPTOR_MANAGER_HPP

#include <cstdint>
#include <vector>

#include "../../../space_core.hpp"

namespace SE::Render::Descriptor
{

using SeDescriptorID     = uint32_t;
using SeDescriptorHandle = void*;

enum class DescriptorType : uint32_t
{
    UNIFORM_BUFFER,
    STORAGE_BUFFER,
    COMBINED_IMAGE_SAMPLER,
    SAMPLED_IMAGE,
    STORAGE_IMAGE
};

struct DescriptorBindingDesc
{
    uint32_t       binding    = 0;
    DescriptorType type       = DescriptorType::COMBINED_IMAGE_SAMPLER;
    uint32_t       count      = 1;
    uint32_t       stageFlags = 0;

    void* imageView = nullptr;
    void* sampler   = nullptr;

    void*    buffer = nullptr;
    uint64_t offset = 0;
    uint64_t range  = 0;
};

struct DescriptorDesc
{
    std::vector<DescriptorBindingDesc> bindings;
};

class IDescriptorManager
{
   public:
    virtual ~IDescriptorManager() = default;

    virtual SeDescriptorID     createDescriptor(DescriptorDesc desc)          = 0;
    virtual SeResult           destroyDescriptor(SeDescriptorID descriptorId) = 0;
    virtual SeDescriptorHandle getDescriptor(SeDescriptorID descriptorId)     = 0;

   private:
};

}  // namespace SE::Render::Descriptor

#endif /* IDESCRIPTOR_MANAGER_HPP */