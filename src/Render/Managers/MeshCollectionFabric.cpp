#include "MeshCollectionFabric.hpp"

#include <cassert>

namespace SE
{

SeMeshCollectionID MeshCollectionFabric::createCollection(size_t maxVertexBytes, size_t maxIndexBytes)
{
    assert(m_bufferManager && "Set BufferManager before creating collections!");

    m_collections.emplace_back(m_bufferManager, maxVertexBytes, maxIndexBytes);

    return static_cast<SeMeshCollectionID>(m_collections.size() - 1);
}

MeshCollection* MeshCollectionFabric::getCollection(SeMeshCollectionID id)
{
    if (id >= m_collections.size())
        return nullptr;

    return &m_collections[id];
}

}  // namespace SE