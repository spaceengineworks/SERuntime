#ifndef FRAME_GRAPH_H
#define FRAME_GRAPH_H

#include <algorithm>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "../RHI/RHI.hpp"
#include "FrameAllocator.hpp"
#include "FramePass.hpp"
#include "IPassParameters.hpp"

namespace SE
{
class RHI;

template <typename Value>
class SmallFlatMap
{
   public:
    Value& operator[](uint64_t key)
    {
        for (auto& e : m_entries)
            if (e.first == key)
                return e.second;
        m_entries.emplace_back(key, Value {});
        return m_entries.back().second;
    }

    const Value* find(uint64_t key) const
    {
        for (const auto& e : m_entries)
            if (e.first == key)
                return &e.second;
        return nullptr;
    }

    void reserve(size_t n)
    {
        m_entries.reserve(n);
    }

    void clear()
    {
        m_entries.clear();
    }

   private:
    std::vector<std::pair<uint64_t, Value>> m_entries;
};

struct FrameGraphOutput
{
    ResourceType    type;
    uint32_t        handle;
    ThsvsAccessType finalAccess;
};

class FrameGraph
{
   public:
    using ResourceResolver = std::function<SeResourseHandle(uint32_t handle)>;

    FrameGraph(RHI* context);
    ~FrameGraph() = default;

    void execute();
    void build();

    template <typename T>
    void add(std::string passName, GPUFlags passflags, T* passParameters, std::function<void(RHI*)> passCallback)
    {
        auto resources = passParameters->getResources();
        m_passes.emplace_back(std::move(passName), passflags, std::move(resources), std::move(passCallback));
    }

    void markOutput(ResourceType type, uint32_t handle, ThsvsAccessType finalAccess)
    {
        m_outputs.push_back({type, handle, finalAccess});
    }

    void registerResolver(ResourceType type, ResourceResolver resolver)
    {
        m_resolvers[type] = std::move(resolver);
    }

   private:
    std::vector<FramePass>        m_passes;
    std::vector<uint32_t>         m_executionOrder;
    std::vector<FrameGraphOutput> m_outputs;
    RHI*                          m_context = nullptr;

    std::unordered_map<ResourceType, ResourceResolver> m_resolvers;

    struct ResourceStateKey
    {
        ResourceType type;
        void*        physical;

        bool operator==(const ResourceStateKey& other) const
        {
            return type == other.type && physical == other.physical;
        }
    };

    struct ResourceStateKeyHash
    {
        size_t operator()(const ResourceStateKey& key) const
        {
            return std::hash<void*> {}(key.physical) ^ (static_cast<size_t>(key.type) << 1);
        }
    };

    std::unordered_map<ResourceStateKey, ThsvsAccessType, ResourceStateKeyHash> m_resourceState;

    static constexpr uint64_t resourceKey(const ResourceField& field) noexcept
    {
        return (static_cast<uint64_t>(field.type) << 32) | field.handle;
    }

    const FrameGraphOutput* findOutput(ResourceType type, uint32_t handle) const
    {
        for (const auto& out : m_outputs)
            if (out.type == type && out.handle == handle)
                return &out;
        return nullptr;
    }

    std::vector<uint32_t> computeAlivePasses(const SmallFlatMap<std::vector<uint32_t>>& producers) const;
};

}  // namespace SE

#endif /* FRAME_GRAPH_H */