#ifndef RHI_H
#define RHI_H

namespace SE
{

enum SeRenderType
{
    Vulkan
};

using SeRender = SeRenderType;

enum SeResultInternals
{
    SE_SUCCESS = 0,
    SE_FAILED  = 1
};

using SeResult       = SeResultInternals;
using SeRenderHandle = void*;

class RHI
{
   public:
    virtual ~RHI()                      = default;
    virtual SeRenderHandle passConfig() = 0;
    // virtual SeResult        CreateViewPortImage()        = 0;
    // virtual SeResult        createOffscreenRenderPass()  = 0;
    // virtual SeResult        createOffscreenFramebuffer() = 0;
    // virtual SeResult        registerAsTexture()          = 0;
};
}  // namespace SE

#endif /* RHI_H */