#ifndef IBUFFER_MANAGER_HPP
#define IBUFFER_MANAGER_HPP

#include <cstdint>
#include <span>
#include <vector>

#include "../../../space_core.hpp"

namespace SE::Render::Buffer
{

using SeBufferID     = uint32_t;
using SeBufferHandle = void*;

enum class BufferMemoryType
{
    DEVICE_LOCAL,
    HOST_VISIBLE,
};

enum class BufferUsage : uint32_t
{
    NONE         = 0,
    TRANSFER_SRC = 1 << 0,
    TRANSFER_DST = 1 << 1,
    UNIFORM      = 1 << 2,
    STORAGE      = 1 << 3,
    INDEX        = 1 << 4,
    VERTEX       = 1 << 5,
    INDIRECT     = 1 << 6
};

inline BufferUsage operator|(BufferUsage a, BufferUsage b)
{
    return static_cast<BufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline BufferUsage operator&(BufferUsage a, BufferUsage b)
{
    return static_cast<BufferUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline BufferUsage& operator|=(BufferUsage& a, BufferUsage b)
{
    a = a | b;
    return a;
}

struct BufferDesc
{
    uint64_t         size;
    BufferUsage      usage;
    BufferMemoryType memoryType = BufferMemoryType::HOST_VISIBLE;
};

class IBufferManager
{
   public:
    virtual ~IBufferManager() = default;

    virtual SeBufferID     createBuffer(BufferDesc desc)                                                       = 0;
    virtual SeResult       populateBuffer(SeBufferID bufferId, uint32_t offset, std::span<const uint8_t> data) = 0;
    virtual SeResult       destroyBuffer(SeBufferID bufferId)                                                  = 0;
    virtual SeBufferHandle getBufferHandle(SeBufferID bufferId)                                                = 0;

   private:
};

}  // namespace SE::Render::Buffer

#endif /* IBUFFER_MANAGER_HPP */