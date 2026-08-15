#ifndef VK_PIPELINE_MANAGER_HPP
#define VK_PIPELINE_MANAGER_HPP

#include <vector>

#include "../../../Managers/DescriptorManager/IDescriptorManager.hpp"
#include "../../../Managers/ShaderManager/IShaderManager.hpp"
#include "../../../RHI/Vulkan/VulkanConfig.hpp"
#include "../IPipelineManager.hpp"

namespace SE::Render::Pipeline
{

struct Pipeline
{
    VkPipeline       pipeline       = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
};

class VkPipelineManager : public IPipelineManager
{
   public:
    VkPipelineManager();
    ~VkPipelineManager() override;

    virtual SePipelineID     createPipeline(PipelineDesc desc) override;
    virtual SeResult         destroyPipeline(SePipelineID PipelineId) override;
    virtual SePipelineHandle getPipelineHandle(SePipelineID PipelineId) override;

    void setConfig(SharedVulkanConfig* config)
    {
        if (config)
            m_config = config;
    }

    void setShaderManager(SE::Render::Shader::IShaderManager* shaderManager)
    {
        if (shaderManager)
            m_shaderManager = shaderManager;
    }

    void setDescriptorManager(SE::Render::Descriptor::IDescriptorManager* mgr)
    {
        if (mgr)
            m_descriptorManager = mgr;
    }

    void setVkRenderPass(void* renderPass) override
    {
        if (renderPass)
            m_renderPass = *static_cast<VkRenderPass*>(renderPass);
    }

   private:
    SharedVulkanConfig*                         m_config            = nullptr;
    VkRenderPass                                m_renderPass        = VK_NULL_HANDLE;
    SE::Render::Shader::IShaderManager*         m_shaderManager     = nullptr;
    SE::Render::Descriptor::IDescriptorManager* m_descriptorManager = nullptr;

    std::vector<Pipeline> m_pipelines;
    SePipelineID          m_nextId = 0;

    void createGraphicsPipeline(PipelineDesc& desc);

    static VkPolygonMode toVkPolygonMode(PolygonMode mode)
    {
        switch (mode)
        {
            case PolygonMode::FILL:
                return VK_POLYGON_MODE_FILL;
            case PolygonMode::LINE:
                return VK_POLYGON_MODE_LINE;
            case PolygonMode::POINT:
                return VK_POLYGON_MODE_POINT;
            default:
                return VK_POLYGON_MODE_FILL;
        }
    }

    static VkCullModeFlagBits toVkCullMode(CullMode mode)
    {
        switch (mode)
        {
            case CullMode::NONE:
                return VK_CULL_MODE_NONE;
            case CullMode::FRONT:
                return VK_CULL_MODE_FRONT_BIT;
            case CullMode::BACK:
                return VK_CULL_MODE_BACK_BIT;
            case CullMode::FRONT_AND_BACK:
                return VK_CULL_MODE_FRONT_AND_BACK;
            default:
                return VK_CULL_MODE_NONE;
        }
    }

    static VkFrontFace toVkFrontFace(FrontFace face)
    {
        switch (face)
        {
            case FrontFace::CLOCKWISE:
                return VK_FRONT_FACE_CLOCKWISE;
            case FrontFace::COUNTER_CLOCKWISE:
                return VK_FRONT_FACE_COUNTER_CLOCKWISE;
            default:
                return VK_FRONT_FACE_CLOCKWISE;
        }
    }

    static VkPrimitiveTopology toVkPrimitiveTopology(PrimitiveTopology topology)
    {
        switch (topology)
        {
            case PrimitiveTopology::POINT_LIST:
                return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            case PrimitiveTopology::LINE_LIST:
                return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            case PrimitiveTopology::LINE_STRIP:
                return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
            case PrimitiveTopology::TRIANGLE_LIST:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            case PrimitiveTopology::TRIANGLE_STRIP:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            case PrimitiveTopology::TRIANGLE_FAN:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
            default:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        }
    }

    static VkShaderStageFlags toShaderStages(ShaderStage stage)
    {
        VkShaderStageFlags vkFlags = 0;
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::VERTEX))
            vkFlags |= VK_SHADER_STAGE_VERTEX_BIT;
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::FRAGMENT))
            vkFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
        if (static_cast<uint32_t>(stage) & static_cast<uint32_t>(ShaderStage::COMPUTE))
            vkFlags |= VK_SHADER_STAGE_COMPUTE_BIT;
        return vkFlags;
    }

    static VkShaderStageFlagBits toVkShaderStage(SE::Render::Shader::SeShaderType type)
    {
        switch (type)
        {
            case SE::Render::Shader::vertex_shader:
                return VK_SHADER_STAGE_VERTEX_BIT;
            case SE::Render::Shader::fragment_shader:
                return VK_SHADER_STAGE_FRAGMENT_BIT;
            default:
                throw std::runtime_error("unknown shader type");
        }
    }
};

}  // namespace SE::Render::Pipeline

#endif /* VK_PIPELINE_MANAGER_HPP */