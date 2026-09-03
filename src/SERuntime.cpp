// TODO: remove
// clang-format off
#define STB_IMAGE_IMPLEMENTATION
#include "ThirdParty/stb_image.h"
#include <glm/gtc/matrix_transform.hpp>
// clang-format on
//
#include <iostream>

#include "ThirdParty/stb_loader.hpp"

//
#include <iostream>
#include <stdexcept>
#include <tracy/Tracy.hpp>

#include "Render/Managers/BufferManager/Vulkan/VkBufferManager.hpp"
#include "Render/Managers/DescriptorManager/Vulkan/VkDescriptorManager.hpp"
#include "Render/Managers/PipelineManager/Vulkan/VkPipelineManager.hpp"
#include "Render/Managers/ShaderManager/Vulkan/VkShaderManager.hpp"
#include "Render/Managers/TextureManager/Vulkan/VkTextureManager.hpp"
#include "Render/RHI/Vulkan/VulkanDevice.hpp"
#include "SERuntime.hpp"

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
    std::unique_ptr<IShaderManager>     shaderManager;
    std::unique_ptr<ITextureManager>    textureManager;
    std::unique_ptr<IPipelineManager>   pipelineManager;
    std::unique_ptr<IDescriptorManager> descriptorManager;
    std::unique_ptr<IBufferManager>     bufferManager;
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
            managers.shaderManager     = std::make_unique<VkShaderManager>();
            managers.textureManager    = std::make_unique<VkTextureManager>();
            managers.pipelineManager   = std::make_unique<VkPipelineManager>();
            managers.descriptorManager = std::make_unique<VkDescriptorManager>();
            managers.bufferManager     = std::make_unique<VkBufferManager>();

            static_cast<VkShaderManager*>(managers.shaderManager.get())->setConfig(config);
            static_cast<VkTextureManager*>(managers.textureManager.get())->setConfig(config);
            static_cast<VkPipelineManager*>(managers.pipelineManager.get())->setConfig(config);
            static_cast<VkDescriptorManager*>(managers.descriptorManager.get())->setConfig(config);
            static_cast<VkBufferManager*>(managers.bufferManager.get())->setConfig(config);

            static_cast<VkPipelineManager*>(managers.pipelineManager.get())->setShaderManager(managers.shaderManager.get());
            static_cast<VkPipelineManager*>(managers.pipelineManager.get())->setDescriptorManager(managers.descriptorManager.get());

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
    m_bufferManager     = std::move(backend.bufferManager);

    if (m_API_render == Vulkan)
    {
        m_context->createOffscreenRenderPass();
        auto* vkDevice = static_cast<VulkanDevice*>(m_context.get());
        m_pipelineManager->setVkRenderPass(vkDevice->getRenderPassPtr());
    }

    m_frameGraph = std::make_unique<FrameGraph>(m_context.get());
    if (!m_frameGraph)
        throw std::runtime_error("frame graph not init.");

    m_meshFabric = std::make_unique<MeshCollectionFabric>();
    m_meshFabric->setBufferManager(m_bufferManager.get());
    m_meshFabric->setConfig(config);

    m_frameGraph->registerResolver(ResourceType::texture,
                                   [this](uint32_t handle) -> SeResourseHandle
                                   {
                                       if (handle == VIEWPORT_DEFAULT_HANDLE)
                                           return m_context->getViewportImageHandle(*m_context->getCurrentFrameIndex());

                                       auto* texData = static_cast<TextureData*>(m_textureManager->getTextureHandle(handle));
                                       return texData ? texData->image : nullptr;
                                   });

    m_frameGraph->markOutput(ResourceType::texture, VIEWPORT_DEFAULT_HANDLE, THSVS_ACCESS_FRAGMENT_SHADER_READ_SAMPLED_IMAGE_OR_UNIFORM_TEXEL_BUFFER);
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
                          ctx->beginRenderPass();
                      });

    static SeShaderID     s_vertId     = SE_INVALID_SHADER_ID;
    static SeShaderID     s_fragId     = SE_INVALID_SHADER_ID;
    static SePipelineID   s_pipelineId = SE_INVALID_PIPELINE_ID;
    static SeTextureID    s_textureId  = SE_INVALID_TEXTURE_ID;
    static SeDescriptorID s_descId     = SE_INVALID_DESCRIPTOR_ID;

    if (s_textureId == SE_INVALID_TEXTURE_ID)
    {
        TextureDesc defaultTexture {};

        int            _width, _height, _channels;
        unsigned char* data = stbi_load("C:/Users/ilanv/Downloads/good.jpg", &_width, &_height, &_channels, 4);

        defaultTexture.imageType = 1;
        defaultTexture.format    = 37;
        defaultTexture.width     = _width;
        defaultTexture.height    = _height;
        defaultTexture.depth     = 1;

        defaultTexture.mipLevels   = 1;
        defaultTexture.levelCount  = 1;
        defaultTexture.arrayLayers = 1;
        defaultTexture.layerCount  = 1;

        defaultTexture.sampleCount   = 1;
        defaultTexture.tiling        = 0;
        defaultTexture.sharingMode   = 0;
        defaultTexture.initialLayout = 0;

        defaultTexture.usage = 20 | 2;

        defaultTexture.pixelData = data;
        defaultTexture.dataSize  = static_cast<size_t>(_width) * _height * 4;

        s_textureId = m_textureManager->createTexture(defaultTexture);

        stbi_image_free(data);

        if (s_textureId == SE_INVALID_TEXTURE_ID)
            throw std::runtime_error("failed to create default texture!");

        DescriptorDesc bindDesc {};

        DescriptorBindingDesc bind1 {};
        auto*                 texData = static_cast<TextureData*>(m_textureManager->getTextureHandle(s_textureId));

        bind1.imageView  = texData->imageView;
        bind1.sampler    = texData->sampler;
        bind1.binding    = 0;
        bind1.type       = DescriptorType::COMBINED_IMAGE_SAMPLER;
        bind1.stageFlags = static_cast<uint32_t>(ShaderStage::FRAGMENT);

        bindDesc.bindings.push_back(bind1);

        /* storage buffer and transform buffer */
        DescriptorBindingDesc bind2 {};
        bind2.binding    = 1;
        bind2.type       = DescriptorType::STORAGE_BUFFER;
        bind2.buffer     = nullptr;
        bind2.stageFlags = static_cast<uint32_t>(ShaderStage::VERTEX);

        bindDesc.bindings.push_back(bind2);

        DescriptorBindingDesc bind3 {};
        bind3.binding    = 2;
        bind3.type       = DescriptorType::STORAGE_BUFFER;
        bind3.buffer     = nullptr;
        bind3.stageFlags = static_cast<uint32_t>(ShaderStage::VERTEX);

        bindDesc.bindings.push_back(bind3);

        s_descId = m_descriptorManager->createDescriptor(bindDesc);

        if (s_descId == SE_INVALID_DESCRIPTOR_ID)
            throw std::runtime_error("failed to create texture descriptor!");

        const char* vertSource = R"(
        #version 450
        #extension GL_ARB_shader_draw_parameters : require

        layout(location = 0) in vec3 inPosition;
        layout(location = 0) out vec2 fragUV;

        layout(push_constant) uniform PushConstants
        {
            mat4 viewProj;
        } push;

        struct MeshInstanceData {
            uint transformIndex;
        };

        layout(std430, set = 0, binding = 1) readonly buffer StorageBuffer
        {
            MeshInstanceData meshInstance[];
        } storage;

        layout(std430, set = 0, binding = 2) readonly buffer TransformBuffer
        {
            mat4 models[];
        } transforms;

        void main()
        {
            uint meshIndex = gl_BaseInstanceARB;

           MeshInstanceData instanceData = storage.meshInstance[meshIndex];

            uint transformIdx  = instanceData.transformIndex;

            mat4 modelMatrix = transforms.models[transformIdx];

            vec4 worldPosition = modelMatrix * vec4(inPosition, 1.0);

            gl_Position = push.viewProj * worldPosition;

            gl_Position.z = gl_Position.z * 0.5 + 0.5;

            const vec2 uvs[12] = vec2[](
                vec2(0.0, 0.0),
                vec2(0.0, 1.0),
                vec2(1.0, 1.0),

                vec2(0.0, 0.0),
                vec2(1.0, 1.0),
                vec2(1.0, 0.0),

                vec2(0.0, 0.0),
                vec2(0.0, 1.0),
                vec2(1.0, 1.0),

                vec2(0.0, 0.0),
                vec2(1.0, 1.0),
                vec2(1.0, 0.0)
            );

            fragUV = uvs[gl_VertexIndex % 12];
        }
        )";

        const char* fragSource = R"(
        #version 450
        #extension GL_ARB_shader_draw_parameters : require

        layout(location = 0) in vec2 fragUV;

        layout(location = 0) out vec4 outColor;

        layout(set = 0, binding = 0) uniform sampler2D uTexture;

        void main()
        {
            outColor = texture(uTexture, fragUV);
        }
        )";

        s_vertId = m_shaderManager->createShader({vertSource, "temp_vert.spv", ShaderType::vertex_shader});
        s_fragId = m_shaderManager->createShader({fragSource, "temp_frag.spv", ShaderType::fragment_shader});

        PipelineDesc pdesc {};
        pdesc.DescriptorId = s_descId;

        pdesc.shaders    = {s_vertId, s_fragId};
        pdesc.stageCount = 2;
        pdesc.flags      = FlagBits::DEPTH_TEST_ENABLE | FlagBits::DEPTH_WRITE_ENABLE;

        pdesc.vertexLayout.stride = sizeof(float) * 3;
        pdesc.vertexLayout.attributes.push_back({.location = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 0});

        pdesc.topology    = PrimitiveTopology::TRIANGLE_LIST;
        pdesc.polygonMode = PolygonMode::FILL;
        pdesc.cullMode    = CullMode::NONE;

        pdesc.sampleCount = SampleCount::SAMPLE_COUNT_1;

        pdesc.depthCompareOp = CompareOp::LESS;

        pdesc.viewportCount = 1;
        pdesc.scissorCount  = 1;
        pdesc.dynamicStates = {DynamicState::VIEWPORT, DynamicState::SCISSOR};

        pdesc.pushConstantStages = ShaderStage::VERTEX;
        pdesc.pushConstantSize   = sizeof(float) * 16;

        pdesc.colorWriteMask      = ColorComponentBits::COLOR_COMPONENT_ALL;
        pdesc.srcColorBlendFactor = BlendFactor::ONE;
        pdesc.dstColorBlendFactor = BlendFactor::ZERO;
        pdesc.colorBlendOp        = BlendOp::ADD;

        s_pipelineId = m_pipelineManager->createPipeline(pdesc);

        if (s_pipelineId == SE_INVALID_PIPELINE_ID)
            throw std::runtime_error("failed to create triangle pipeline!");
    }

    PassDrawParameters* draw = m_frameAllocator.allocateFrameParams<PassDrawParameters>();
    draw->textureHandle      = s_textureId;
    draw->descHandle         = s_descId;

    auto*    pipelineMgr   = m_pipelineManager.get();
    auto*    descriptorMgr = m_descriptorManager.get();
    auto*    meshMgr       = m_meshFabric.get();
    uint32_t pipelineId    = s_pipelineId;

    /* all code here simulates like engine acuatly load mesh to scene two main buffer one indices and vertex for MDI draws */

    // const float vertices[] = {-0.4f, -0.4f, -0.3f, -0.4f, 0.4f, -0.3f, 0.4f, 0.4f,  -0.3f,

    //                           -0.4f, -0.4f, -0.3f, 0.4f,  0.4f, -0.3f, 0.4f, -0.4f, -0.3f,

    //                           -0.4f, -0.4f, 0.3f,  -0.4f, 0.4f, 0.3f,  0.4f, 0.4f,  0.3f,

    //                           -0.4f, -0.4f, 0.3f,  0.4f,  0.4f, 0.3f,  0.4f, -0.4f, 0.3f};

    // const uint32_t indices[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

    // std::span<const uint8_t> vertexData {reinterpret_cast<const uint8_t*>(vertices), sizeof(vertices)};

    // std::span<const uint8_t> indexData {reinterpret_cast<const uint8_t*>(indices), sizeof(indices)};

    ObjMesh mesh = loadObj("C:/Users/ilanv/Downloads/stanford-bunny.obj");

    std::span<const uint8_t> vertexData {reinterpret_cast<const uint8_t*>(mesh.vertices.data()), mesh.vertices.size() * sizeof(float)};

    std::span<const uint8_t> indexData {reinterpret_cast<const uint8_t*>(mesh.indices.data()), mesh.indices.size() * sizeof(uint32_t)};

    SeMeshCollectionID MeshCollectionID = m_meshFabric->createCollection(1000, 1000);

    MeshCollection* collection = m_meshFabric->getCollection(MeshCollectionID);

    uint32_t idx = collection->addMesh(vertexData, indexData, static_cast<uint32_t>(mesh.indices.size()), sizeof(float) * 3, 0);

    /* load mvp for this mesh */
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix           = glm::scale(modelMatrix, glm::vec3(4.0f));

    std::span<const uint8_t> transformData {reinterpret_cast<const uint8_t*>(&modelMatrix), sizeof(glm::mat4)};

    collection->updateMeshData(WhichType::TRANSFORMS, idx, transformData);

    DescriptorDesc        bindDesc {};
    DescriptorBindingDesc update1Bind {};

    update1Bind.binding = 1;
    update1Bind.type    = DescriptorType::STORAGE_BUFFER;
    update1Bind.buffer  = static_cast<Buffer*>(collection->getStorageBufferHandle())->buffer;
    update1Bind.range   = VK_WHOLE_SIZE;

    bindDesc.bindings.push_back(update1Bind);

    DescriptorBindingDesc update2Bind {};

    update2Bind.binding = 2;
    update2Bind.type    = DescriptorType::STORAGE_BUFFER;
    update2Bind.buffer  = static_cast<Buffer*>(collection->getTransformsBufferHandle())->buffer;
    update2Bind.range   = VK_WHOLE_SIZE;

    bindDesc.bindings.push_back(update2Bind);

    if (m_descriptorManager->updateDescriptor(s_descId, bindDesc) != SE_SUCCESS)
        std::cout << "updateDescriptor didnt update descriptor" << std::endl;

    // uint32_t idx = collection->addMesh(vertexData, indexData, static_cast<uint32_t>(std::size(indices)), sizeof(float) * 3, 0);

    std::cout << idx << std::endl;

    draw->collectionHandle = MeshCollectionID;

    /* ------------------------- */

    m_frameGraph->add("forward_pass", GPUFlags::Render, draw,
                      [meshMgr, pipelineMgr, descriptorMgr, pipelineId, draw, idx](RHI* ctx)
                      {
                          /* temp */
                          meshMgr->getCollection(draw->collectionHandle)->flushAsyncUploads();

                          static glm::mat4 modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(3.5f));
                          modelMatrix                  = glm::rotate(modelMatrix, glm::radians(0.1f), glm::normalize(glm::vec3(1.0f, 0.7f, 0.4f)));

                          std::span<const uint8_t> transformData {reinterpret_cast<const uint8_t*>(&modelMatrix), sizeof(glm::mat4)};

                          meshMgr->getCollection(draw->collectionHandle)->updateMeshData(WhichType::TRANSFORMS, idx, transformData);

                          meshMgr->getCollection(draw->collectionHandle)->buildDrawCommands();

                          ctx->bindPipe(pipelineMgr->getPipelineHandle(pipelineId));
                          ctx->bindVertexBuffers(meshMgr->getCollection(draw->collectionHandle));
                          ctx->bindIndexBuffer(meshMgr->getCollection(draw->collectionHandle));

                          ctx->setViewport();
                          ctx->setScissor();

                          ctx->pushConstants(pipelineMgr->getPipelineHandle(pipelineId), static_cast<uint32_t>(ShaderStage::VERTEX), 0, sizeof(draw->mvp), draw->mvp);

                          ctx->bindDescriptorSet(descriptorMgr->getDescriptor(draw->descHandle), pipelineMgr->getPipelineHandle(pipelineId));

                          ctx->DrawIndexedIndirect(meshMgr->getCollection(draw->collectionHandle));
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