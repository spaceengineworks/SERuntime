#ifndef FRAME_ALLOCATOR_H
#define FRAME_ALLOCATOR_H

#include <cassert>
#include <cstdint>
#include <vector>

#define DEFAULT_POOL_SIZE 1024

namespace SE
{

class FrameAllocator
{
    using resourseRef = uint32_t;

   public:
    FrameAllocator() : m_pool(DEFAULT_POOL_SIZE)
    {
    }
    ~FrameAllocator() = default;

    template <typename T, typename... Args>
    T* allocateFrameParams(Args&&... args)
    {
        void* memory = &m_pool[m_resourceIdx];
        m_resourceIdx += sizeof(T);
        return new (memory) T(std::forward<Args>(args)...);
    }

    constexpr void reset() noexcept
    {
        m_resourceIdx = 0;
    }

   private:
    std::vector<uint8_t> m_pool;
    resourseRef          m_resourceIdx = 0;
};
}  // namespace SE

#endif /* FRAME_ALLOCATOR_H */