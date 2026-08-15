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

struct VertexAttributeDesc
{
    uint32_t location = 0;
    uint32_t format   = 0;
    uint32_t offset   = 0;
};

struct VertexLayoutDesc
{
    // TODO: add input rate.
    uint32_t                         stride     = 0;
    std::vector<VertexAttributeDesc> attributes = {};
};

}  // namespace SE::Render

#endif /* RENDER_CORE_HPP */