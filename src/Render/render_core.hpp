#ifndef RENDER_CORE_HPP
#define RENDER_CORE_HPP

namespace SE::Render
{

struct Vertex
{
    float         x, y;
    unsigned char r, g, b, a;
};

struct PushConstants
{
    float screenWidth;
    float screenHeight;
};

int constexpr SIZE_OF_ATTRIBUTES = 1;

size_t constexpr POSITION_POS = offsetof(Vertex, x);

enum
{
    POSITION_IN_ATTRIBUTES = 0,
    COLOR_IN_ATTRIBUTES    = 1,
    UV_IN_ATTRIBUTES       = 2,
    WH_IN_ATTRIBUTES       = 3
};

}  // namespace SE::Render

#endif /* RENDER_CORE_HPP */