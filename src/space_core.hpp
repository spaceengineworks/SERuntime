#ifndef SPACE_CORE_H
#define SPACE_CORE_H

#define CHECK_FLAG(flags, bit) ((flags & bit) ? true : false)
#define SET_FLAG(flags, bit)   (flags |= bit)
#define CLEAR_FLAG(flags, bit) (flags &= ~bit)

enum SeResultInternals
{
    SE_SUCCESS                    = 0,
    SE_FAILED                     = 1,
    SE_RESIZED                    = 2,
    SE_FAILED_TO_DESTROY          = UINT32_MAX,
    SE_INVALID_TEXTURE_ID         = UINT32_MAX,
    SE_INVALID_SHADER_ID          = UINT32_MAX,
    SE_INVALID_PIPELINE_ID        = UINT32_MAX,
    SE_INVALID_DESCRIPTOR_ID      = UINT32_MAX,
    SE_INVALID_BUFFER_ID          = UINT32_MAX,
    SE_INVALID_MESH_COLLECTION_ID = UINT32_MAX,
    SE_INVALID_MESH_ID            = UINT32_MAX
};

using SeResult = SeResultInternals;

#endif /* SPACE_CORE_H */