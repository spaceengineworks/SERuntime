add_library(Physics

)
target_include_directories(Physics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Physics PUBLIC Jolt::Jolt)