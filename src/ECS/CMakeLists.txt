add_library(ECS

)
target_include_directories(ECS PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ECS PUBLIC EnTT::EnTT)