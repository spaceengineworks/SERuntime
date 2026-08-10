#ifndef VK_SHADER_MANAGER_H
#define VK_SHADER_MANAGER_H

#include <assert.h>

#include <queue>
#include <vector>
#include <shaderc/shaderc.hpp>

#include "../IShaderManager.hpp"
#include "../../../RHI/Vulkan/VulkanConfig.hpp"

namespace SE::Render::Shader
{

struct Shader
{
    VkShaderModule       module;
    std::vector<uint8_t> source;
    const char*          fileName;
    SeShaderType         shaderType;
};

class VkShaderManager : public IShaderManager
{
   public:
    VkShaderManager();
    ~VkShaderManager() override;

    virtual SeShaderID     createShader(ShaderDesc desc) override;
    virtual SeResult       destroyShader(SeShaderID shaderId) override;
    virtual SeShaderHandle getShaderHandle(SeShaderID shaderId) override;

    void setConfig(SharedVulkanConfig* config)
    {
        if (config)
            m_config = config;
    }

   private:
    /* using types */
    using FreeShaderIdHeap = std::priority_queue<SeShaderID, std::vector<SeShaderID>, std::greater<SeShaderID>>;

    /* --------------- */

    SharedVulkanConfig* m_config = nullptr;

    std::vector<Shader> m_shaders;
    FreeShaderIdHeap    m_freeShaderIds;
    SeShaderID          m_nextId = 0;

    shaderc::Compiler       m_compiler;
    shaderc::CompileOptions m_options;

    std::vector<uint8_t> compileShader(const char* source, const char* fileName, SeShaderType shaderType);
    shaderc_shader_kind  toShaderc(SeShaderType type) const
    {
        const shaderc_shader_kind shaderTypes[] = {shaderc_vertex_shader, shaderc_fragment_shader};
        return shaderTypes[type];
    }
};

}  // namespace SE::Render::Shader

#endif /* VK_SHADER_MANAGER_H */