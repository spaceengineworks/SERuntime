#ifndef MESH_COLLECTION_FABRIC_HPP
#define MESH_COLLECTION_FABRIC_HPP

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "BufferManager/IBufferManager.hpp"
#include "MeshCollection.hpp"

namespace SE
{

class MeshCollectionFabric
{
   public:
    MeshCollectionFabric()  = default;
    ~MeshCollectionFabric() = default;

    inline void setBufferManager(IBufferManager* bufferManager)
    {
        m_bufferManager = bufferManager;
    }

    SeMeshCollectionID createCollection(size_t maxVertexBytes, size_t maxIndexBytes);

    MeshCollection* getCollection(SeMeshCollectionID id);

   private:
    IBufferManager*             m_bufferManager = nullptr;
    std::vector<MeshCollection> m_collections;
};

}  // namespace SE

#endif /* MESH_COLLECTION_FABRIC_HPP */