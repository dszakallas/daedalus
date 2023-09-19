#include <metal_stdlib>

#include "ShaderTypes.hh"

using namespace metal;

namespace Scenes {
    namespace S13E02 {
        
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
                     constant vector_float2 *vertices [[buffer(VertexInputIndex::Vertices)]],
                     constant float4x4 *cam [[buffer(VertexInputIndex::Cam)]],
                     constant float4x4 *clip [[buffer(VertexInputIndex::Clip)]],
                     constant vector_float3 *color [[buffer(VertexInputIndex::Color)]])
        {
            RasterizerData out;
            

            float4 obj = float4(vertices[vertexID].xy, 0, 1);
            out.position = (*clip * *cam) * obj;
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
    } /* namespace S13E02 */
} /* namespace Scenes */
