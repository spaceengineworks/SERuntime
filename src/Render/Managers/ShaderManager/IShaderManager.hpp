#ifndef ISHADER_MANAGER_H
#define ISHADER_MANAGER_H

#include <cstdint>

#include "../../../space_core.hpp"

namespace SE
{

using SeShaderID     = uint32_t;
using SeShaderHandle = void*;

enum ShaderType
{
    vertex_shader   = 0,
    fragment_shader = 1
};

using SeShaderType = ShaderType;

struct ShaderDesc
{
    const char*  source;
    const char*  fileName;
    SeShaderType shaderType;
};

class IShaderManager
{
   public:
    virtual ~IShaderManager() = default;

    virtual SeShaderID     createShader(ShaderDesc desc)        = 0;
    virtual SeResult       destroyShader(SeShaderID shaderId)   = 0;
    virtual SeShaderHandle getShaderHandle(SeShaderID shaderId) = 0;

   private:
};

}  // namespace SE

#endif /* ISHADER_MANAGER_H */