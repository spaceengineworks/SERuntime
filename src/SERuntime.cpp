#include "SERuntime.hpp"

#include <iostream>
#include <stdexcept>
#include <tracy/Tracy.hpp>

#include "Render/Managers/DescriptorManager/Vulkan/VkDescriptorManager.hpp"
#include "Render/Managers/PipelineManager/Vulkan/VkPipelineManager.hpp"
#include "Render/Managers/ShaderManager/Vulkan/VkShaderManager.hpp"
#include "Render/Managers/TextureManager/Vulkan/VkTextureManager.hpp"
#include "Render/RHI/Vulkan/VulkanDevice.hpp"

namespace SE
{
namespace detail
{
std::unique_ptr<RHI> createRenderContext(SeRender render)
{
    switch (render)
    {
        case SeRender::Vulkan:
            return std::make_unique<VulkanDevice>();
        default:
            throw std::runtime_error("unsupported render backend");
    }
}

struct BackendManagers
{
    std::unique_ptr<Render::Shader::IShaderManager>         shaderManager;
    std::unique_ptr<Render::Texture::ITextureManager>       textureManager;
    std::unique_ptr<Render::Pipeline::IPipelineManager>     pipelineManager;
    std::unique_ptr<Render::Descriptor::IDescriptorManager> descriptorManager;
};

BackendManagers createBackendManagers(SeRender render, SharedVulkanConfig* config)
{
    if (!config)
        throw std::runtime_error("failed to retrieve backend config");

    switch (render)
    {
        case SeRender::Vulkan:
        {
            BackendManagers managers {};
            managers.shaderManager     = std::make_unique<Render::Shader::VkShaderManager>();
            managers.textureManager    = std::make_unique<Render::Texture::VkTextureManager>();
            managers.pipelineManager   = std::make_unique<Render::Pipeline::VkPipelineManager>();
            managers.descriptorManager = std::make_unique<Render::Descriptor::VkDescriptorManager>();

            static_cast<Render::Shader::VkShaderManager*>(managers.shaderManager.get())->setConfig(config);
            static_cast<Render::Texture::VkTextureManager*>(managers.textureManager.get())->setConfig(config);
            static_cast<Render::Pipeline::VkPipelineManager*>(managers.pipelineManager.get())->setConfig(config);
            static_cast<Render::Descriptor::VkDescriptorManager*>(managers.descriptorManager.get())->setConfig(config);

            static_cast<Render::Pipeline::VkPipelineManager*>(managers.pipelineManager.get())->setShaderManager(managers.shaderManager.get());
            static_cast<Render::Pipeline::VkPipelineManager*>(managers.pipelineManager.get())->setDescriptorManager(managers.descriptorManager.get());

            return managers;
        }
        default:
            throw std::runtime_error("unsupported backend managers");
    }
}
}  // namespace detail

SERuntime::SERuntime(SeRender render) : m_API_render(render)
{
    m_context = detail::createRenderContext(m_API_render);
}

void SERuntime::initEngine()
{
    auto* config  = static_cast<SharedVulkanConfig*>(m_context->passConfig());
    auto  backend = detail::createBackendManagers(m_API_render, config);

    m_shaderManager     = std::move(backend.shaderManager);
    m_textureManager    = std::move(backend.textureManager);
    m_pipelineManager   = std::move(backend.pipelineManager);
    m_descriptorManager = std::move(backend.descriptorManager);

    if (m_API_render == Vulkan)
    {
        m_context->createOffscreenRenderPass();
        auto* vkDevice = static_cast<VulkanDevice*>(m_context.get());
        m_pipelineManager->setVkRenderPass(vkDevice->getRenderPassPtr());
    }

    const char* vertSource = R"(
    #version 450

    vec2 positions[3] = vec2[](
        vec2( 0.0, -0.5),
        vec2( 0.5,  0.5),
        vec2(-0.5,  0.5)
    );

    void main() {
        gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    }
)";

    const char* fragSource = R"(
    #version 450

    layout(location = 0) out vec4 outColor;

    void main() {
        outColor = vec4(1.0, 0.5, 0.0, 1.0);
    }
)";

    auto vertId = m_shaderManager->createShader({vertSource, "temp_vert.spv", Render::Shader::vertex_shader});
    auto fragId = m_shaderManager->createShader({fragSource, "temp_frag.spv", Render::Shader::fragment_shader});

    Render::Pipeline::PipelineDesc desc {};

    // Render::VertexLayoutDesc vertexLayout {};
    // vertexLayout.stride     = sizeof(Render::Vertex);
    // vertexLayout.attributes = {
    //     {/*location*/ 0, /*format*/ static_cast<uint32_t>(VK_FORMAT_R32G32_SFLOAT), offsetof(Render::Vertex, x)},
    //     {/*location*/ 1, /*format*/ static_cast<uint32_t>(VK_FORMAT_R8G8B8A8_UNORM), offsetof(Render::Vertex, r)},
    // };

    // desc.vertexLayout = vertexLayout;

    desc.shaders    = {vertId, fragId};
    desc.stageCount = 2;
    desc.flags      = 0;

    desc.topology    = Render::Pipeline::PrimitiveTopology::TRIANGLE_LIST;
    desc.polygonMode = Render::Pipeline::PolygonMode::FILL;
    desc.cullMode    = Render::Pipeline::CullMode::NONE;

    desc.sampleCount = Render::Pipeline::SampleCount::SAMPLE_COUNT_1;

    desc.viewportCount = 1;
    desc.scissorCount  = 1;
    desc.dynamicStates = {Render::Pipeline::DynamicState::VIEWPORT, Render::Pipeline::DynamicState::SCISSOR};

    desc.pushConstantStages = Render::Pipeline::ShaderStage::NONE;
    desc.pushConstantSize   = 0;

    desc.colorWriteMask      = Render::Pipeline::COLOR_COMPONENT_ALL;
    desc.srcColorBlendFactor = Render::Pipeline::BlendFactor::ONE;
    desc.dstColorBlendFactor = Render::Pipeline::BlendFactor::ZERO;
    desc.colorBlendOp        = Render::Pipeline::BlendOp::ADD;

    m_pipelineManager->createPipeline(desc);

    m_frameGraph = std::make_unique<FrameGraph>(m_context.get());
    if (!m_frameGraph)
        throw std::runtime_error("frame graph not init.");
}

SERuntime::~SERuntime() = default;

SeRenderHandle SERuntime::SeAskConfig()
{
    if (!m_context)
        return nullptr;
    return m_context->passConfig();
}

SeResult SERuntime::initViewPort(uint32_t width, uint32_t height)
{
    m_context->CreateViewPortImage(width, height);
    m_context->createOffscreenFramebuffer();
    m_context->viewPortCommandBuffer();
    m_context->createSyncObjects();

    // PassClearParameters* clear = new PassClearParameters();
    PassClearParameters* clear = m_frameAllocator.allocateFrameParams<PassClearParameters>();
    clear->m_clearColor[0]     = 0.1f;
    clear->m_clearColor[1]     = 0.15f;
    clear->m_clearColor[2]     = 0.2f;
    clear->m_clearColor[3]     = 1.0f;

    m_frameGraph->add("clear", GPUFlags::Render, clear,
                      [clear](RHI* ctx)
                      {
                          static float speed[3] = {0.00005f, 0.0002f, 0.0003f};

                          for (int i = 0; i < 3; ++i)
                          {
                              clear->m_clearColor[i] += speed[i];

                              if (clear->m_clearColor[i] >= 1.0f)
                              {
                                  clear->m_clearColor[i] = 1.0f;
                                  speed[i]               = -speed[i];
                              }
                              else if (clear->m_clearColor[i] <= 0.0f)
                              {
                                  clear->m_clearColor[i] = 0.0f;
                                  speed[i]               = -speed[i];
                              }
                          }

                          ctx->setClearColor(clear->m_clearColor[0], clear->m_clearColor[1], clear->m_clearColor[2]);

                          SeResourseHandle viewportImage = ctx->getViewportImageHandle(*ctx->getCurrentFrameIndex());

                          BarrierDesc barrier {};
                          barrier.resourse = viewportImage;
                          barrier.type     = ResourseType::Image;

                          barrier.prevAccesses = {THSVS_ACCESS_FRAGMENT_SHADER_READ_SAMPLED_IMAGE_OR_UNIFORM_TEXEL_BUFFER};
                          barrier.nextAccesses = {THSVS_ACCESS_COLOR_ATTACHMENT_WRITE};

                          barrier.prevLayout = THSVS_IMAGE_LAYOUT_OPTIMAL;
                          barrier.nextLayout = THSVS_IMAGE_LAYOUT_OPTIMAL;

                          barrier.baseMipLevel   = 0;
                          barrier.levelCount     = 1;
                          barrier.baseArrayLayer = 0;
                          barrier.layerCount     = 1;

                          ctx->insertPipelineBarrier(&barrier);

                          ctx->beginRenderPass();
                      });

    auto* pipelineMgr = m_pipelineManager.get();

    m_frameGraph->add("draw_triangle", GPUFlags::Render, clear,
                      [pipelineMgr](RHI* ctx)
                      {
                          ctx->bindPipe(pipelineMgr->getPipelineHandle(0));

                          ctx->setViewport();
                          ctx->setScissor();

                          ctx->callDraw();
                          ctx->endRenderPass();
                      });

    m_frameGraph->build();

    return SeResult::SE_SUCCESS;
}

void SERuntime::updateAndRender()
{
    if (m_context)
    {
        m_frameAllocator.reset();
        m_context->updateAndRender(m_frameGraph.get());
    }
}

SeTextureHandle SERuntime::getViewportTex(uint32_t currentFrame) const
{
    if (!m_context)
        return VK_NULL_HANDLE;

    return m_context->getViewportTex(currentFrame);
}

}  // namespace SE