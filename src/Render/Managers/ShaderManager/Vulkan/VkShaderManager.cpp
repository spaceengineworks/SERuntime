#include "VkShaderManager.hpp"

namespace SE::Render::Shader
{

VkShaderManager::VkShaderManager()
{
}

VkShaderManager::~VkShaderManager()
{
    if (!m_config || !m_config->device)
        return;

    for (Shader& shader : m_shaders)
    {
        if (shader.module != VK_NULL_HANDLE)
            vkDestroyShaderModule(*m_config->device, shader.module, nullptr);
    }
}

SeShaderID VkShaderManager::createShader(ShaderDesc desc)
{
    std::vector<uint32_t> result = compileShader(desc.source, desc.fileName, desc.shaderType);

    VkShaderModuleCreateInfo createInfo {};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = result.size() * sizeof(uint32_t);
    createInfo.pCode    = result.data();

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(*m_config->device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        return SE_INVALID_SHADER_ID;

    Shader shader = {.module = shaderModule, .source = result, .fileName = desc.fileName, .shaderType = desc.shaderType};

    if (!m_freeShaderIds.empty())
    {
        SeShaderID shaderId = m_freeShaderIds.top();
        m_freeShaderIds.pop();

        m_shaders[shaderId] = shader;
        return shaderId;
    }
    m_shaders.push_back(shader);

    return m_nextId++;
}

SeResult VkShaderManager::destroyShader(SeShaderID shaderId)
{
    if (shaderId == SE_INVALID_SHADER_ID || shaderId >= m_shaders.size())
        return SE_FAILED_TO_DESTROY;

    Shader& shader = m_shaders[shaderId];

    if (shader.module == VK_NULL_HANDLE)
        return SE_FAILED_TO_DESTROY;

    vkDestroyShaderModule(*m_config->device, shader.module, nullptr);

    shader.module = VK_NULL_HANDLE;

    m_freeShaderIds.push(shaderId);

    return SE_SUCCESS;
}

SeShaderHandle VkShaderManager::getShaderHandle(SeShaderID shaderId)
{
    if (shaderId == SE_INVALID_SHADER_ID || shaderId >= m_shaders.size())
        return nullptr;

    Shader& shader = m_shaders[shaderId];

    if (shader.module == VK_NULL_HANDLE)
        return nullptr;

    return static_cast<SeShaderHandle>(&shader);
}

/* Methods to compile shaders */

std::vector<uint32_t> VkShaderManager::compileShader(const char* source, const char* fileName, SeShaderType shaderType)
{
    shaderc::SpvCompilationResult result = m_compiler.CompileGlslToSpv(source, toShaderc(shaderType), fileName, m_options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        assert(false && "Bad status after shader compilation.");
    }

    std::vector<uint32_t> spirv_code(result.cbegin(), result.cend());
    return spirv_code;
}

}  // namespace SE::Render::Shader