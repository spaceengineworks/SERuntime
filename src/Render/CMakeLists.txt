add_library(Render
    RHI/RHI.cpp
    RHI/Vulkan/VulkanDevice.cpp
)

target_include_directories(Render PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Render PUBLIC
    Vulkan::Vulkan
    unofficial::shaderc::shaderc
)