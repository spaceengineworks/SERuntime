#ifndef SPACE_CORE_H
#define SPACE_CORE_H

enum SeResultInternals
{
    SE_SUCCESS            = 0,
    SE_FAILED             = 1,
    SE_RESIZED            = 2,
    FAILED_TO_DESTROY     = UINT32_MAX,
    SE_INVALID_TEXTURE_ID = UINT32_MAX,
    SE_INVALID_SHADER_ID  = UINT32_MAX
};

using SeResult = SeResultInternals;

#endif /* SPACE_CORE_H */