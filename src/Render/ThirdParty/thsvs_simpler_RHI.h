#ifndef THSVS_SIMPLER_RHI_H
#define THSVS_SIMPLER_RHI_H

/*
ThsvsAccessType defines all potential resource usages in the Vulkan API.
*/
typedef enum ThsvsAccessType
{
    THSVS_ACCESS_NONE,  // No access. Useful primarily for initialization

    // Read access
    // Requires VK_NV_device_generated_commands to be enabled
    THSVS_ACCESS_COMMAND_BUFFER_READ_NV,                                                     // Command buffer read operation as defined by NV_device_generated_commands
    THSVS_ACCESS_INDIRECT_BUFFER,                                                            // Read as an indirect buffer for drawing or dispatch
    THSVS_ACCESS_INDEX_BUFFER,                                                               // Read as an index buffer for drawing
    THSVS_ACCESS_VERTEX_BUFFER,                                                              // Read as a vertex buffer for drawing
    THSVS_ACCESS_VERTEX_SHADER_READ_UNIFORM_BUFFER,                                          // Read as a uniform buffer in a vertex shader
    THSVS_ACCESS_VERTEX_SHADER_READ_SAMPLED_IMAGE_OR_UNIFORM_TEXEL_BUFFER,                   // Read as a sampled image/uniform texel buffer in a vertex shader
    THSVS_ACCESS_VERTEX_SHADER_READ_OTHER,                                                   // Read as any other resource in a vertex shader
    THSVS_ACCESS_TESSELLATION_CONTROL_SHADER_READ_UNIFORM_BUFFER,                            // Read as a uniform buffer in a tessellation control shader
    THSVS_ACCESS_TESSELLATION_CONTROL_SHADER_READ_SAMPLED_IMAGE_OR_UNIFORM_TEXEL_BUFFER,     // Read as a sampled image/uniform texel buffer  in a tessellation control shader
    THSVS_ACCESS_TESSELLATION_CONTROL_SHADER_READ_OTHER,                                     // Read as any other resource in a tessellation control shader
    THSVS_ACCESS_TESSELLATION_EVALUATION_SHADER_READ_UNIFORM_BUFFER,                         // Read as a uniform buffer in a tessellation evaluation shader
    THSVS_ACCESS_TESSELLATION_EVALUATION_SHADER_READ_SAMPLED_IMAGE_OR_UNIFORM_TEXEL_BUFFER,  // Read as a sampled image/uniform texel buffer in a tessellation evaluation shader
    THSVS_ACCESS_TESSELLATION_EVALUATION_SHADER_READ_OTHER,                                  // Read as any other resource in a tessellation evaluation shader
    THSVS_ACCESS_GEOMETRY_SHADER_READ_UNIFORM_BUFFER,                                        // Read as a uniform buffer in a geometry shader
    THSVS_ACCESS_GEOMETRY_SHADER_READ_SAMPLED_IMAGE_OR_UNIFORM_TEXEL_BUFFER,                 // Read as a sampled image/uniform texel buffer  in a geometry shader
    THSVS_ACCESS_GEOMETRY_SHADER_READ_OTHER,                                                 // Read as any other resource in a geometry shader
    THSVS_ACCESS_TASK_SHADER_READ_UNIFORM_BUFFER_NV,                                         // Read as a uniform buffer in a task shader
    THSVS_ACCESS_TASK_SHADER_READ_SAMPLED_IMAGE_OR_UNIFORM_TEXEL_BUFFER_NV,                  // Read as a sampled image/uniform texel buffer in a task shader
    THSVS_ACCESS_TASK_SHADER_READ_OTHER_NV,                                                  // Read as any other resource in a task shader
    THSVS_ACCESS_MESH_SHADER_READ_UNIFORM_BUFFER_NV,                                         // Read as a uniform buffer in a mesh shader
    THSVS_ACCESS_MESH_SHADER_READ_SAMPLED_IMAGE_OR_UNIFORM_TEXEL_BUFFER_NV,                  // Read as a sampled image/uniform texel buffer in a mesh shader
    THSVS_ACCESS_MESH_SHADER_READ_OTHER_NV,                                                  // Read as any other resource in a mesh shader
    THSVS_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_EXT,                                        // Read as a transform feedback counter buffer
    THSVS_ACCESS_FRAGMENT_DENSITY_MAP_READ_EXT,                                              // Read as a fragment density map image
    THSVS_ACCESS_SHADING_RATE_READ_NV,                                                       // Read as a shading rate image
    THSVS_ACCESS_FRAGMENT_SHADER_READ_UNIFORM_BUFFER,                                        // Read as a uniform buffer in a fragment shader
    THSVS_ACCESS_FRAGMENT_SHADER_READ_SAMPLED_IMAGE_OR_UNIFORM_TEXEL_BUFFER,                 // Read as a sampled image/uniform texel buffer  in a fragment shader
    THSVS_ACCESS_FRAGMENT_SHADER_READ_COLOR_INPUT_ATTACHMENT,                                // Read as an input attachment with a color format in a fragment shader
    THSVS_ACCESS_FRAGMENT_SHADER_READ_DEPTH_STENCIL_INPUT_ATTACHMENT,                        // Read as an input attachment with a depth/stencil format in a fragment shader
    THSVS_ACCESS_FRAGMENT_SHADER_READ_OTHER,                                                 // Read as any other resource in a fragment shader
    THSVS_ACCESS_COLOR_ATTACHMENT_READ,                                                      // Read by standard blending/logic operations or subpass load operations
    THSVS_ACCESS_COLOR_ATTACHMENT_ADVANCED_BLENDING_EXT,                                     // Read by advanced blending, standard blending, logic operations, or subpass load operations
    THSVS_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ,                                              // Read by depth/stencil tests or subpass load operations
    THSVS_ACCESS_COMPUTE_SHADER_READ_UNIFORM_BUFFER,                                         // Read as a uniform buffer in a compute shader
    THSVS_ACCESS_COMPUTE_SHADER_READ_SAMPLED_IMAGE_OR_UNIFORM_TEXEL_BUFFER,                  // Read as a sampled image/uniform texel buffer in a compute shader
    THSVS_ACCESS_COMPUTE_SHADER_READ_OTHER,                                                  // Read as any other resource in a compute shader
    THSVS_ACCESS_ANY_SHADER_READ_UNIFORM_BUFFER,                                             // Read as a uniform buffer in any shader
    THSVS_ACCESS_ANY_SHADER_READ_UNIFORM_BUFFER_OR_VERTEX_BUFFER,                            // Read as a uniform buffer in any shader, or a vertex buffer
    THSVS_ACCESS_ANY_SHADER_READ_SAMPLED_IMAGE_OR_UNIFORM_TEXEL_BUFFER,                      // Read as a sampled image in any shader
    THSVS_ACCESS_ANY_SHADER_READ_OTHER,                                                      // Read as any other resource (excluding attachments) in any shader
    THSVS_ACCESS_TRANSFER_READ,                                                              // Read as the source of a transfer operation
    THSVS_ACCESS_HOST_READ,                                                                  // Read on the host

    // Requires VK_KHR_swapchain to be enabled
    THSVS_ACCESS_PRESENT,  // Read by the presentation engine (i.e. vkQueuePresentKHR)

    // Requires VK_EXT_conditional_rendering to be enabled
    THSVS_ACCESS_CONDITIONAL_RENDERING_READ_EXT,  // Read by conditional rendering

    // Requires VK_NV_ray_tracing to be enabled
    THSVS_ACCESS_RAY_TRACING_SHADER_ACCELERATION_STRUCTURE_READ_NV,  // Read by a ray tracing shader as an acceleration structure
    THSVS_ACCESS_ACCELERATION_STRUCTURE_BUILD_READ_NV,               // Read as an acceleration structure during a build

    // Read accesses end
    THSVS_END_OF_READ_ACCESS,

    // Write access
    // Requires VK_NV_device_generated_commands to be enabled
    THSVS_ACCESS_COMMAND_BUFFER_WRITE_NV,               // Command buffer write operation
    THSVS_ACCESS_VERTEX_SHADER_WRITE,                   // Written as any resource in a vertex shader
    THSVS_ACCESS_TESSELLATION_CONTROL_SHADER_WRITE,     // Written as any resource in a tessellation control shader
    THSVS_ACCESS_TESSELLATION_EVALUATION_SHADER_WRITE,  // Written as any resource in a tessellation evaluation shader
    THSVS_ACCESS_GEOMETRY_SHADER_WRITE,                 // Written as any resource in a geometry shader

    // Requires VK_NV_mesh_shading to be enabled
    THSVS_ACCESS_TASK_SHADER_WRITE_NV,  // Written as any resource in a task shader
    THSVS_ACCESS_MESH_SHADER_WRITE_NV,  // Written as any resource in a mesh shader

    // Requires VK_EXT_transform_feedback to be enabled
    THSVS_ACCESS_TRANSFORM_FEEDBACK_WRITE_EXT,          // Written as a transform feedback buffer
    THSVS_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_EXT,  // Written as a transform feedback counter buffer

    THSVS_ACCESS_FRAGMENT_SHADER_WRITE,           // Written as any resource in a fragment shader
    THSVS_ACCESS_COLOR_ATTACHMENT_WRITE,          // Written as a color attachment during rendering, or via a subpass store op
    THSVS_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE,  // Written as a depth/stencil attachment during rendering, or via a subpass store op

    // Requires VK_KHR_maintenance2 to be enabled
    THSVS_ACCESS_DEPTH_ATTACHMENT_WRITE_STENCIL_READ_ONLY,  // Written as a depth aspect of a depth/stencil attachment during rendering, whilst the stencil aspect is read-only
    THSVS_ACCESS_STENCIL_ATTACHMENT_WRITE_DEPTH_READ_ONLY,  // Written as a stencil aspect of a depth/stencil attachment during rendering, whilst the depth aspect is read-only

    THSVS_ACCESS_COMPUTE_SHADER_WRITE,  // Written as any resource in a compute shader
    THSVS_ACCESS_ANY_SHADER_WRITE,      // Written as any resource in any shader
    THSVS_ACCESS_TRANSFER_WRITE,        // Written as the destination of a transfer operation
    THSVS_ACCESS_HOST_PREINITIALIZED,   // Data pre-filled by host before device access starts
    THSVS_ACCESS_HOST_WRITE,            // Written on the host

    // Requires VK_NV_ray_tracing to be enabled
    THSVS_ACCESS_ACCELERATION_STRUCTURE_BUILD_WRITE_NV,  // Written as an acceleration structure during a build

    THSVS_ACCESS_COLOR_ATTACHMENT_READ_WRITE,  // Read or written as a color attachment during rendering
                                               // General access
    THSVS_ACCESS_GENERAL,                      // Covers any access - useful for debug, generally avoid for performance reasons

    // Number of access types
    THSVS_NUM_ACCESS_TYPES
} ThsvsAccessType;

typedef enum ThsvsImageLayout
{
    THSVS_IMAGE_LAYOUT_OPTIMAL,  // Choose the most optimal layout for each usage. Performs layout transitions as appropriate for the access.
    THSVS_IMAGE_LAYOUT_GENERAL,  // Layout accessible by all Vulkan access types on a device - no layout transitions except for presentation

    // Requires VK_KHR_shared_presentable_image to be enabled. Can only be used for shared presentable images (i.e. single-buffered swap chains).
    THSVS_IMAGE_LAYOUT_GENERAL_AND_PRESENTATION  // As GENERAL, but also allows presentation engines to access it - no layout transitions
} ThsvsImageLayout;

#endif /* THSVS_SIMPLER_RHI_H */