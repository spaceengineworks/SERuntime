#ifndef MESH_COLLECTION_FABRIC_HPP
#define MESH_COLLECTION_FABRIC_HPP

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "../RHI/Vulkan/VulkanConfig.hpp"
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
        if (bufferManager)
            m_bufferManager = bufferManager;
    }

    inline void setConfig(SharedVulkanConfig* config)
    {
        if (config)
            m_config = config;
    }

    SeMeshCollectionID createCollection(size_t maxVertexBytes, size_t maxIndexBytes);

    MeshCollection* getCollection(SeMeshCollectionID id);

   private:
    IBufferManager*             m_bufferManager = nullptr;
    SharedVulkanConfig*         m_config        = nullptr;
    std::vector<MeshCollection> m_collections;
};

}  // namespace SE

#endif /* MESH_COLLECTION_FABRIC_HPP */