#include "VkPipelineManager.hpp"

#include "../../../Managers/DescriptorManager/Vulkan/VkDescriptorManager.hpp"
#include "../../../Managers/ShaderManager/Vulkan/VkShaderManager.hpp"

namespace SE::Render::Pipeline
{

VkPipelineManager::VkPipelineManager() = default;

VkPipelineManager::~VkPipelineManager()
{
    if (!m_config || !m_config->device)
        return;

    for (Pipeline& pipeline : m_pipelines)
    {
        if (pipeline.pipeline != VK_NULL_HANDLE)
            vkDestroyPipeline(*m_config->device, pipeline.pipeline, nullptr);

        if (pipeline.pipelineLayout != VK_NULL_HANDLE)
            vkDestroyPipelineLayout(*m_config->device, pipeline.pipelineLayout, nullptr);
    }
}

SePipelineID VkPipelineManager::createPipeline(PipelineDesc desc)
{
    if (!m_config || !m_config->device || !m_shaderManager || !m_renderPass)
        return SE_INVALID_PIPELINE_ID;

    createGraphicsPipeline(desc);
    return m_nextId++;
}

SeResult VkPipelineManager::destroyPipeline(SePipelineID pipelineId)
{
    if (pipelineId == SE_INVALID_PIPELINE_ID || pipelineId >= m_pipelines.size())
        return FAILED_TO_DESTROY;

    Pipeline pipeline = m_pipelines[pipelineId];

    if (pipeline.pipeline == VK_NULL_HANDLE || pipeline.pipelineLayout == VK_NULL_HANDLE)
        return FAILED_TO_DESTROY;

    vkDestroyPipeline(*m_config->device, pipeline.pipeline, nullptr);
    vkDestroyPipelineLayout(*m_config->device, pipeline.pipelineLayout, nullptr);

    pipeline.pipeline       = VK_NULL_HANDLE;
    pipeline.pipelineLayout = VK_NULL_HANDLE;

    return SE_SUCCESS;
}

SePipelineHandle VkPipelineManager::getPipelineHandle(SePipelineID pipelineId)
{
    if (pipelineId == SE_INVALID_PIPELINE_ID || pipelineId >= m_pipelines.size())
        return nullptr;

    Pipeline& pipeline = m_pipelines[pipelineId];

    if (pipeline.pipeline == VK_NULL_HANDLE || pipeline.pipelineLayout == VK_NULL_HANDLE)
        return nullptr;

    return reinterpret_cast<SePipelineHandle>(&pipeline);
}

/* Methods for pipeline */

void VkPipelineManager::createGraphicsPipeline(PipelineDesc& desc)
{
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    for (SeShaderID shaderId : desc.shaders)
    {
        SeShaderHandle              handle = m_shaderManager->getShaderHandle(shaderId);
        SE::Render::Shader::Shader* shader = static_cast<SE::Render::Shader::Shader*>(handle);

        if (!shader)
            assert(false && "Error bad shader module!");

        VkPipelineShaderStageCreateInfo shaderStageInfo {};
        shaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.stage  = toVkShaderStage(shader->shaderType);
        shaderStageInfo.module = shader->module;
        shaderStageInfo.pName  = "main";

        shaderStages.push_back(shaderStageInfo);
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = 0;
    vertexInputInfo.pVertexBindingDescriptions      = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions    = nullptr;

    VkVertexInputBindingDescription                bindingDescription {};
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

    if (!desc.vertexLayout.attributes.empty())
    {
        bindingDescription.binding   = 0;
        bindingDescription.stride    = desc.vertexLayout.stride;
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;  // TODO: add to the struct field to enable instancing.

        attributeDescriptions.reserve(desc.vertexLayout.attributes.size());
        for (const auto& attr : desc.vertexLayout.attributes)
        {
            VkVertexInputAttributeDescription vkAttr {};
            vkAttr.binding  = 0;
            vkAttr.location = attr.location;
            vkAttr.format   = static_cast<VkFormat>(attr.format);
            vkAttr.offset   = attr.offset;
            attributeDescriptions.push_back(vkAttr);
        }
    }

    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = desc.vertexLayout.attributes.empty() ? 0 : 1;
    vertexInputInfo.pVertexBindingDescriptions      = desc.vertexLayout.attributes.empty() ? nullptr : &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.empty() ? nullptr : attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
    inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology               = toVkPrimitiveTopology(desc.topology);
    inputAssembly.primitiveRestartEnable = CHECK_FLAG(desc.flags, PRIMITIVE_RESTART);

    VkPipelineViewportStateCreateInfo viewportState {};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = desc.viewportCount;
    viewportState.scissorCount  = desc.scissorCount;

    VkPipelineRasterizationStateCreateInfo rasterizer {};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = CHECK_FLAG(desc.flags, CLAMP_BIT);
    rasterizer.rasterizerDiscardEnable = CHECK_FLAG(desc.flags, DISCARD_BIT);
    rasterizer.polygonMode             = toVkPolygonMode(desc.polygonMode);
    rasterizer.lineWidth               = desc.lineWidth;
    rasterizer.cullMode                = toVkCullMode(desc.cullMode);
    rasterizer.frontFace               = toVkFrontFace(desc.frontFace);
    rasterizer.depthBiasEnable         = CHECK_FLAG(desc.flags, DEPTH_BIAS_BIT);
    rasterizer.depthBiasConstantFactor = desc.depthBiasConstantFactor;
    rasterizer.depthBiasClamp          = desc.depthBiasClamp;
    rasterizer.depthBiasSlopeFactor    = desc.depthBiasSlopeFactor;

    VkPipelineMultisampleStateCreateInfo multisampling {};
    multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable   = CHECK_FLAG(desc.flags, SHADING_BIT);
    multisampling.rasterizationSamples  = static_cast<VkSampleCountFlagBits>(desc.sampleCount);
    multisampling.minSampleShading      = desc.minSampleShading;
    multisampling.pSampleMask           = (desc.sampleMask != 0xFFFFFFFF) ? &desc.sampleMask : nullptr;
    multisampling.alphaToCoverageEnable = CHECK_FLAG(desc.flags, ALPHA_TO_COVERAGE_BIT);
    multisampling.alphaToOneEnable      = CHECK_FLAG(desc.flags, ALPHA_TO_ONE_BIT);

    VkPipelineDepthStencilStateCreateInfo depthStencil {};
    // TODO: Complete.

    VkPipelineColorBlendAttachmentState colorBlendAttachment {};
    colorBlendAttachment.colorWriteMask      = desc.colorWriteMask;
    colorBlendAttachment.blendEnable         = CHECK_FLAG(desc.flags, BLEND_ENABLE_BIT);
    colorBlendAttachment.srcColorBlendFactor = static_cast<VkBlendFactor>(desc.srcColorBlendFactor);
    colorBlendAttachment.dstColorBlendFactor = static_cast<VkBlendFactor>(desc.dstColorBlendFactor);
    colorBlendAttachment.colorBlendOp        = static_cast<VkBlendOp>(desc.colorBlendOp);
    colorBlendAttachment.srcAlphaBlendFactor = static_cast<VkBlendFactor>(desc.srcAlphaBlendFactor);
    colorBlendAttachment.dstAlphaBlendFactor = static_cast<VkBlendFactor>(desc.dstAlphaBlendFactor);
    colorBlendAttachment.alphaBlendOp        = static_cast<VkBlendOp>(desc.alphaBlendOp);

    VkPipelineColorBlendStateCreateInfo colorBlending {};
    colorBlending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable     = VK_FALSE;
    colorBlending.logicOp           = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount   = 1;
    colorBlending.pAttachments      = &colorBlendAttachment;
    colorBlending.blendConstants[0] = desc.blendConstants[0];
    colorBlending.blendConstants[1] = desc.blendConstants[1];
    colorBlending.blendConstants[2] = desc.blendConstants[2];
    colorBlending.blendConstants[3] = desc.blendConstants[3];

    std::vector<VkDynamicState> dynamicStates;
    for (const auto& state : desc.dynamicStates)
    {
        dynamicStates.push_back(static_cast<VkDynamicState>(state));
    }

    VkPipelineDynamicStateCreateInfo dynamicState {};
    dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates    = dynamicStates.data();

    VkDescriptorSetLayout setLayout = VK_NULL_HANDLE;

    if (desc.DescriptorId != SE_INVALID_DESCRIPTOR_ID)
    {
        auto* handle = static_cast<Descriptor::Descriptor*>(m_descriptorManager->getDescriptor(desc.DescriptorId));
        if (!handle)
            throw std::runtime_error("invalid descriptor id passed to pipeline");

        setLayout = handle->layout;
    }

    VkPushConstantRange pushConstantRange = {};
    uint32_t            pushConstantCount = 0;

    if (desc.pushConstantSize > 0)
    {
        pushConstantRange.stageFlags = toShaderStages(desc.pushConstantStages);
        pushConstantRange.offset     = desc.pushConstantOffset;
        pushConstantRange.size       = desc.pushConstantSize;
        pushConstantCount            = 1;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
    pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount         = (setLayout != VK_NULL_HANDLE) ? 1 : 0;
    pipelineLayoutInfo.pSetLayouts            = (setLayout != VK_NULL_HANDLE) ? &setLayout : nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = pushConstantCount;
    pipelineLayoutInfo.pPushConstantRanges    = (pushConstantCount > 0) ? &pushConstantRange : nullptr;

    Pipeline pipeline;
    if (vkCreatePipelineLayout(*m_config->device, &pipelineLayoutInfo, nullptr, &pipeline.pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo {};
    pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount          = desc.stageCount;
    pipelineInfo.pStages             = shaderStages.data();
    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    // TODO: added in future: pipelineInfo.pTessellationState =
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pDepthStencilState  = nullptr;  //&depthStencil;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDynamicState       = &dynamicState;

    pipelineInfo.layout     = pipeline.pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass    = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex  = -1;

    if (vkCreateGraphicsPipelines(*m_config->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline.pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    m_pipelines.push_back(pipeline);
}

}  // namespace SE::Render::Pipeline