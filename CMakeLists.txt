cmake_minimum_required(VERSION 3.20)
project(SERuntime LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_PROFILING "Enable Tracy profiler" ON)
set(ENABLE_PROFILING ${ENABLE_PROFILING} CACHE BOOL "" FORCE)

include(FetchContent)

FetchContent_Declare(
    tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG        v0.13.1
)

FetchContent_MakeAvailable(tracy)
    
find_package(Vulkan REQUIRED MODULE)
find_package(unofficial-shaderc CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Jolt CONFIG REQUIRED)

# add_subdirectory(src/Physics)
add_subdirectory(src/Render)
# add_subdirectory(src/ECS)

set(COMMON_SOURCES
    src/SERuntime.cpp
)

add_library(SERuntime SHARED ${COMMON_SOURCES})

target_include_directories(SERuntime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_definitions(SERuntime PRIVATE SERUNTIME_EXPORTS)
 
set_target_properties(SERuntime PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    CXX_RTTI ON
)

target_link_libraries(SERuntime PRIVATE
    Render
    # Physics
    # ECS

    EnTT::EnTT
    glm::glm
    Jolt::Jolt
)

target_include_directories(SERuntime PUBLIC ${tracy_SOURCE_DIR}/public)

if(SERUNTIME_ENABLE_PROFILING)
    target_link_libraries(SERuntime PUBLIC Tracy::TracyClient)
    target_compile_definitions(SERuntime PUBLIC TRACY_ENABLE TRACY_API_EXPORT)
endif()

if(MSVC)
    target_compile_options(SERuntime PRIVATE /arch:AVX2)
else()
    target_compile_options(SERuntime PRIVATE -mavx2)
endif()