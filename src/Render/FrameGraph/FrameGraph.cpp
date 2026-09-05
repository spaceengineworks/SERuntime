#include "FrameGraph.hpp"

#include <cassert>
#include <iostream>

namespace SE
{

FrameGraph::FrameGraph(RHI* context) : m_context(context)
{
}

std::vector<uint32_t> FrameGraph::computeAlivePasses(const SmallFlatMap<std::vector<uint32_t>>& producers) const
{
    const uint32_t        passCount = static_cast<uint32_t>(m_passes.size());
    std::vector<bool>     visited(passCount, false);
    std::vector<uint32_t> stack;

    if (m_outputs.empty())
    {
        std::vector<uint32_t> all(passCount);
        for (uint32_t i = 0; i < passCount; ++i) all[i] = i;
        return all;
    }

    for (const auto& out : m_outputs)
    {
        uint64_t key = (static_cast<uint64_t>(out.type) << 32) | out.handle;
        if (const auto* p = producers.find(key))
            for (uint32_t idx : *p) stack.push_back(idx);
    }

    while (!stack.empty())
    {
        uint32_t idx = stack.back();
        stack.pop_back();
        if (visited[idx])
            continue;
        visited[idx] = true;

        for (const auto& field : m_passes[idx].reads())
        {
            uint64_t key = resourceKey(field);
            if (const auto* p = producers.find(key))
                for (uint32_t producerIdx : *p)
                    if (!visited[producerIdx])
                        stack.push_back(producerIdx);
        }
    }

    std::vector<uint32_t> alive;
    alive.reserve(passCount);
    for (uint32_t i = 0; i < passCount; ++i)
        if (visited[i])
            alive.push_back(i);
    return alive;
}

void FrameGraph::build()
{
    const uint32_t passCount = static_cast<uint32_t>(m_passes.size());

    SmallFlatMap<std::vector<uint32_t>> producers;
    producers.reserve(passCount * 2);

    for (uint32_t i = 0; i < passCount; ++i)
        for (const auto& field : m_passes[i].writes()) producers[resourceKey(field)].push_back(i);

    std::vector<uint32_t> alive = computeAlivePasses(producers);
    std::vector<bool>     isAlive(passCount, false);
    for (uint32_t idx : alive) isAlive[idx] = true;

    std::vector<std::vector<uint32_t>> adjacency(passCount);
    std::vector<uint32_t>              inDegree(passCount, 0);

    for (uint32_t i = 0; i < passCount; ++i)
    {
        if (!isAlive[i])
            continue;

        for (const auto& field : m_passes[i].reads())
        {
            uint64_t    key  = resourceKey(field);
            const auto* prod = producers.find(key);
            if (!prod)
                continue;

            for (uint32_t producerIdx : *prod)
            {
                if (producerIdx == i || !isAlive[producerIdx])
                    continue;
                adjacency[producerIdx].push_back(i);
                ++inDegree[i];
            }
        }
    }

    std::vector<uint32_t> ready;
    for (uint32_t idx : alive)
        if (inDegree[idx] == 0)
            ready.insert(std::upper_bound(ready.begin(), ready.end(), idx), idx);

    std::vector<uint32_t> order;
    order.reserve(alive.size());

    while (!ready.empty())
    {
        uint32_t current = ready.front();
        ready.erase(ready.begin());
        order.push_back(current);

        for (uint32_t next : adjacency[current])
            if (--inDegree[next] == 0)
                ready.insert(std::upper_bound(ready.begin(), ready.end(), next), next);
    }

    if (order.size() != alive.size())
    {
        assert(false && "FrameGraph::build: cyclic dependency between passes");
        m_executionOrder.clear();
        return;
    }

    m_executionOrder = std::move(order);
}

void FrameGraph::execute()
{
    for (uint32_t idx : m_executionOrder)
    {
        FramePass& pass = m_passes[idx];

        for (const ResourceField& field : pass.m_passParameters)
        {
            auto resolverIt = m_resolvers.find(field.type);
            if (resolverIt == m_resolvers.end())
                continue;

            SeResourseHandle physical = resolverIt->second(field.handle);
            if (!physical)
                continue;

            ResourceStateKey key {field.type, physical};

            if (field.isRenderTarget)
            {
                const FrameGraphOutput* output = findOutput(field.type, field.handle);
                m_resourceState[key]           = output ? output->finalAccess : inferDefaultGpuAccess(field.type, field.access);
                continue;
            }

            ThsvsAccessType nextAccess = field.gpuAccess != THSVS_ACCESS_NONE ? field.gpuAccess : inferDefaultGpuAccess(field.type, field.access);

            auto it = m_resourceState.find(key);

            if (it == m_resourceState.end())
            {
                if (field.access == ResourceAccess::Write)
                {
                    BarrierDesc barrier {};
                    barrier.resourse        = physical;
                    barrier.type            = (field.type == ResourceType::texture) ? ResourseType::Image : ResourseType::Buffer;
                    barrier.prevAccesses    = {THSVS_ACCESS_NONE};
                    barrier.nextAccesses    = {nextAccess};
                    barrier.discardContents = true;
                    m_context->insertPipelineBarrier(&barrier);
                }
            }
            else if (it->second != nextAccess)
            {
                BarrierDesc barrier {};
                barrier.resourse     = physical;
                barrier.type         = (field.type == ResourceType::texture) ? ResourseType::Image : ResourseType::Buffer;
                barrier.prevAccesses = {it->second};
                barrier.nextAccesses = {nextAccess};
                m_context->insertPipelineBarrier(&barrier);
            }

            m_resourceState[key] = nextAccess;
        }

        pass.execute(m_context);
    }
}

}  // namespace SE