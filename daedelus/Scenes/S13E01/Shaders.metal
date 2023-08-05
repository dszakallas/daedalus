#include <metal_stdlib>

// Include header shared between this Metal shader code and C code executing Metal API commands.
#include "../../Engine/ShaderTypes.h"

using namespace metal;

// Vertex shader outputs and fragment shader inputs
struct RasterizerData
{
    // The [[position]] attribute of this member indicates that this value
    // is the clip space position of the vertex when this structure is
    // returned from the vertex function.
    float4 position [[position]];

    // Since this member does not have a special attribute, the rasterizer
    // interpolates its value with the values of the other triangle vertices
    // and then passes the interpolated value to the fragment shader for each
    // fragment in the triangle.
    float4 color;
};

vertex RasterizerData
vertexShader(uint vertexID [[vertex_id]],
             constant vector_float2 *vertices [[buffer(VertexInputIndexVertices)]],
             constant vector_float2 *viewportSizePointer [[buffer(VertexInputIndexViewportSize)]],
             constant vector_float3 *color [[buffer(VertexInputIndexColor)]])
{
    RasterizerData out;

    // Index into the array of positions to get the current vertex.
    // The positions are specified in pixel dimensions (i.e. a value of 100
    // is 100 pixels from the origin).
    float2 pixelSpacePosition = vertices[vertexID].xy;

    // Halve the viewport size
    vector_float2 halfViewportSize = *viewportSizePointer / 2.0;

    // To convert from positions in pixel space to positions in clip-space,
    //  divide the pixel coordinates by half the size of the viewport.
    out.position = vector_float4(0.0, 0.0, 0.0, 1.0);
    out.position.xy = (pixelSpacePosition - halfViewportSize) / halfViewportSize;
    out.position.z = 0.0;

    // Pass the input color directly to the rasterizer.
    out.color = vector_float4(0, 0, 0, 0);
    out.color.rgb = *color;

    return out;
}

fragment float4 fragmentShader(RasterizerData in [[stage_in]])
{
    // Return the interpolated color.
    return in.color;
}
