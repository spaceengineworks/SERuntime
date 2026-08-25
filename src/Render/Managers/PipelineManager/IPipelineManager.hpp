#ifndef IPIPELINE_MANAGER_HPP
#define IPIPELINE_MANAGER_HPP

#include <assert.h>

#include <cstdint>
#include <vector>

#include "../../../space_core.hpp"
#include "../../render_core.hpp"
#include "../ShaderManager/IShaderManager.hpp"

namespace SE
{

using SePipelineID     = uint32_t;
using SePipelineHandle = void*;

enum class PolygonMode : uint32_t
{
    FILL,
    LINE,
    POINT
};

enum class CullMode : uint32_t
{
    NONE,
    FRONT,
    BACK,
    FRONT_AND_BACK
};

enum class FrontFace : uint32_t
{
    CLOCKWISE,
    COUNTER_CLOCKWISE
};

enum class PrimitiveTopology : uint32_t
{
    POINT_LIST,
    LINE_LIST,
    LINE_STRIP,
    TRIANGLE_LIST,
    TRIANGLE_STRIP,
    TRIANGLE_FAN
};

enum class SampleCount : uint32_t
{
    SAMPLE_COUNT_1  = 1,
    SAMPLE_COUNT_2  = 2,
    SAMPLE_COUNT_4  = 4,
    SAMPLE_COUNT_8  = 8,
    SAMPLE_COUNT_16 = 16,
    SAMPLE_COUNT_32 = 32,
    SAMPLE_COUNT_64 = 64
};

enum class BlendFactor : uint32_t
{
    ZERO,
    ONE,
    SRC_COLOR,
    ONE_MINUS_SRC_COLOR,
    DST_COLOR,
    ONE_MINUS_DST_COLOR,
    SRC_ALPHA,
    ONE_MINUS_SRC_ALPHA,
    DST_ALPHA,
    ONE_MINUS_DST_ALPHA,
    CONSTANT_COLOR,
    ONE_MINUS_CONSTANT_COLOR,
    CONSTANT_ALPHA,
    ONE_MINUS_CONSTANT_ALPHA,
    SRC_ALPHA_SATURATE,
    SRC1_COLOR,
    ONE_MINUS_SRC1_COLOR,
    SRC1_ALPHA,
    ONE_MINUS_SRC1_ALPHA
};

enum class BlendOp : uint32_t
{
    ADD,
    SUBTRACT,
    REVERSE_SUBTRACT,
    MIN,
    MAX
};

enum class CompareOp : uint32_t
{
    NEVER,
    LESS,
    EQUAL,
    LESS_OR_EQUAL,
    GREATER,
    NOT_EQUAL,
    GREATER_OR_EQUAL,
    ALWAYS
};

enum ColorComponentBits : uint32_t
{
    COLOR_COMPONENT_R_BIT = 1 << 0,
    COLOR_COMPONENT_G_BIT = 1 << 1,
    COLOR_COMPONENT_B_BIT = 1 << 2,
    COLOR_COMPONENT_A_BIT = 1 << 3,
    COLOR_COMPONENT_ALL   = (1 << 4) - 1
};

enum class DynamicState : uint32_t
{
    VIEWPORT,
    SCISSOR,
    LINE_WIDTH,
    DEPTH_BIAS,
    BLEND_CONSTANTS,
    DEPTH_BOUNDS,
    STENCIL_COMPARE_MASK,
    STENCIL_WRITE_MASK,
    STENCIL_REFERENCE,
    CULL_MODE,
    FRONT_FACE,
    PRIMITIVE_TOPOLOGY,
    VIEWPORT_WITH_COUNT,
    SCISSOR_WITH_COUNT,
    VERTEX_INPUT_BINDING_STRIDE,
    DEPTH_TEST_ENABLE,
    DEPTH_WRITE_ENABLE,
    DEPTH_COMPARE_OP,
    DEPTH_BOUNDS_TEST_ENABLE,
    STENCIL_TEST_ENABLE,
    STENCIL_OP,
    RASTERIZER_DISCARD_ENABLE,
    DEPTH_BIAS_ENABLE,
    PRIMITIVE_RESTART_ENABLE,
    VIEWPORT_W_SCALING_ENABLE_NV,
    DISCARD_RECTANGLE_ENABLE_EXT,
    SAMPLE_LOCATIONS_ENABLE_EXT,
    SHADING_RATE_IMAGE_ENABLE_NV,
    COVERAGE_TO_COLOR_ENABLE_NV,
    COVERAGE_TO_COLOR_LOCATION_NV,
    COVERAGE_MODULATION_MODE_NV,
    COVERAGE_MODULATION_TABLE_ENABLE_NV,
    COVERAGE_MODULATION_TABLE_NV,
    SHADING_RATE_NV,
    REPRESENTATIVE_FRAGMENT_TEST_ENABLE_NV,
    ATTACHMENT_FEEDBACK_LOOP_ENABLE_EXT
};

enum FlagBits : uint32_t
{
    CLAMP_BIT               = 1 << 0,
    DISCARD_BIT             = 1 << 1,
    DEPTH_BIAS_BIT          = 1 << 2,
    PRIMITIVE_RESTART       = 1 << 3,
    SHADING_BIT             = 1 << 4,
    ALPHA_TO_COVERAGE_BIT   = 1 << 5,
    ALPHA_TO_ONE_BIT        = 1 << 6,
    BLEND_ENABLE_BIT        = 1 << 7,
    DEPTH_TEST_ENABLE       = 1 << 8,
    DEPTH_WRITE_ENABLE      = 1 << 11,
    DEPTH_BOUND_TEST_ENABLE = 1 << 12,
    STENCIL_TEST_ENABLE     = 1 << 13
};

enum class ShaderStage : uint32_t
{
    NONE     = 0,
    VERTEX   = 1 << 0,
    FRAGMENT = 1 << 1,
    COMPUTE  = 1 << 2
};

inline ShaderStage operator|(ShaderStage a, ShaderStage b)
{
    return static_cast<ShaderStage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

struct PipelineDesc
{
    uint32_t DescriptorId = static_cast<uint32_t>(SE_INVALID_DESCRIPTOR_ID);

    std::vector<SeShaderID> shaders = {};
    uint32_t                stageCount;
    uint32_t                flags = 0;

    Render::VertexLayoutDesc vertexLayout = {};

    PolygonMode polygonMode             = PolygonMode::FILL;
    float       lineWidth               = 1.0f;
    CullMode    cullMode                = CullMode::NONE;
    FrontFace   frontFace               = FrontFace::CLOCKWISE;
    float       depthBiasConstantFactor = 0.0f;
    float       depthBiasClamp          = 0.0f;
    float       depthBiasSlopeFactor    = 0.0f;

    PrimitiveTopology topology = PrimitiveTopology::TRIANGLE_LIST;

    uint32_t viewportCount = 1;
    uint32_t scissorCount  = 1;

    SampleCount sampleCount      = SampleCount::SAMPLE_COUNT_1;
    float       minSampleShading = 1.0f;
    uint32_t    sampleMask       = 0xFFFFFFFF;

    CompareOp depthCompareOp = CompareOp::LESS;
    float     minDepthBounds = 0.0f; /* 0.0-1.0 */
    float     maxDepthBounds = 0.0f; /* 0.0-1.0 */

    uint32_t    colorWriteMask      = COLOR_COMPONENT_ALL;
    BlendFactor srcColorBlendFactor = BlendFactor::SRC_ALPHA;
    BlendFactor dstColorBlendFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;
    BlendOp     colorBlendOp        = BlendOp::ADD;
    BlendFactor srcAlphaBlendFactor = BlendFactor::ONE;
    BlendFactor dstAlphaBlendFactor = BlendFactor::ZERO;
    BlendOp     alphaBlendOp        = BlendOp::ADD;
    float       blendConstants[4]   = {0.0f, 0.0f, 0.0f, 0.0f};

    std::vector<DynamicState> dynamicStates = {};

    ShaderStage pushConstantStages = ShaderStage::NONE;
    uint32_t    pushConstantOffset = 0;
    uint32_t    pushConstantSize   = 0;
};

class IPipelineManager
{
   public:
    virtual ~IPipelineManager() = default;

    virtual SePipelineID     createPipeline(PipelineDesc desc)          = 0;
    virtual SeResult         destroyPipeline(SePipelineID pipelineId)   = 0;
    virtual SePipelineHandle getPipelineHandle(SePipelineID pipelineId) = 0;

    virtual void setVkRenderPass(void* renderPass)
    {
        /* stub used only for vulkan */
    }

   private:
};

}  // namespace SE

#endif /* IPIPELINE_MANAGER_HPP */