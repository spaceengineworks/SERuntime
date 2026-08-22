add_library(Render
    RHI/Vulkan/VulkanDevice.cpp
    FrameGraph/FrameAllocator.cpp
    FrameGraph/FrameGraph.cpp
    Managers/MeshCollection.cpp
    Managers/MeshCollectionFabric.cpp
    Managers/TextureManager/Vulkan/VkTextureManager.cpp
    Managers/ShaderManager/Vulkan/VkShaderManager.cpp
    Managers/PipelineManager/Vulkan/VkPipelineManager.cpp
    Managers/DescriptorManager/Vulkan/VkDescriptorManager.cpp
    Managers/BufferManager/Vulkan/VkBufferManager.cpp
)

target_include_directories(Render PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${tracy_SOURCE_DIR}/public
)

target_link_libraries(Render PUBLIC
    Vulkan::Vulkan
    unofficial::shaderc::shaderc
    GPUOpen::VulkanMemoryAllocator
)

if(SERUNTIME_ENABLE_PROFILING)
    target_link_libraries(Render PUBLIC Tracy::TracyClient)
    target_compile_definitions(Render PUBLIC TRACY_ENABLE)
endif()