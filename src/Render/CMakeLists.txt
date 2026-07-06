add_library(Render
    RHI/RHI.cpp
    RHI/Vulkan/VulkanDevice.cpp
)

target_include_directories(Render PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${tracy_SOURCE_DIR}/public
)

target_link_libraries(Render PUBLIC
    Vulkan::Vulkan
    unofficial::shaderc::shaderc
)

if(SERUNTIME_ENABLE_PROFILING)
    target_link_libraries(Render PUBLIC Tracy::TracyClient)
    target_compile_definitions(Render PUBLIC TRACY_ENABLE)
endif()