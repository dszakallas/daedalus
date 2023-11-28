#include <AppKit/AppKit.hpp>
#include "AppKitExt.hh"
#include <simd/simd.h>

#include "../../Engine/Engine.hh"
#include "../../Utility/Math.hh"

#include "./ShaderTypes.hh"
#include "./Scene.hh"

namespace Scenes {
namespace NavigateCube {

using namespace NSExt;

void Renderer::buildShaders() {
    NS::Error* err = nullptr;
    // Load all the shader files with a .metal file extension in the project.
    auto library = ns_ptr(device->newDefaultLibrary());
    if (!library) {
        __builtin_printf( "%s", err->localizedDescription()->utf8String() );
        assert( false );
    }

    auto vertexShader = ns_ptr(library->newFunction(NSExt::UTF8String("Scenes::NavigateCube::vertexMain")));
    auto fragmentShader = ns_ptr(library->newFunction(NSExt::UTF8String("Scenes::NavigateCube::fragmentMain")));

    auto desc = ns_ptr(MTL::RenderPipelineDescriptor::alloc()->init());
    desc->setVertexFunction( vertexShader.get() );
    desc->setFragmentFunction( fragmentShader.get() );
    desc->colorAttachments()->object(0)->setPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB );
    desc->setDepthAttachmentPixelFormat( MTL::PixelFormat::PixelFormatDepth16Unorm );

    state = ns_ptr(device->newRenderPipelineState( desc.get(), &err ));
    if (!state) {
        __builtin_printf("%s", err->localizedDescription()->utf8String());
        assert( false );
    }
}

void Renderer::buildDepthStencilStates() {
    auto dsDesc = ns_ptr(MTL::DepthStencilDescriptor::alloc()->init());
    dsDesc->setDepthCompareFunction( MTL::CompareFunction::CompareFunctionLess );
    dsDesc->setDepthWriteEnabled( true );
    depthStencilState = ns_ptr(device->newDepthStencilState( dsDesc.get() ));
}

void Renderer::buildBuffers()
{
    using simd::float3;
    const float s = 0.5f;

    VertexData verts[] = {
        //   Positions          Normals
        { { -s, -s, +s }, { 0.f,  0.f,  1.f } },
        { { +s, -s, +s }, { 0.f,  0.f,  1.f } },
        { { +s, +s, +s }, { 0.f,  0.f,  1.f } },
        { { -s, +s, +s }, { 0.f,  0.f,  1.f } },

        { { +s, -s, +s }, { 1.f,  0.f,  0.f } },
        { { +s, -s, -s }, { 1.f,  0.f,  0.f } },
        { { +s, +s, -s }, { 1.f,  0.f,  0.f } },
        { { +s, +s, +s }, { 1.f,  0.f,  0.f } },

        { { +s, -s, -s }, { 0.f,  0.f, -1.f } },
        { { -s, -s, -s }, { 0.f,  0.f, -1.f } },
        { { -s, +s, -s }, { 0.f,  0.f, -1.f } },
        { { +s, +s, -s }, { 0.f,  0.f, -1.f } },

        { { -s, -s, -s }, { -1.f, 0.f,  0.f } },
        { { -s, -s, +s }, { -1.f, 0.f,  0.f } },
        { { -s, +s, +s }, { -1.f, 0.f,  0.f } },
        { { -s, +s, -s }, { -1.f, 0.f,  0.f } },

        { { -s, +s, +s }, { 0.f,  1.f,  0.f } },
        { { +s, +s, +s }, { 0.f,  1.f,  0.f } },
        { { +s, +s, -s }, { 0.f,  1.f,  0.f } },
        { { -s, +s, -s }, { 0.f,  1.f,  0.f } },

        { { -s, -s, -s }, { 0.f, -1.f,  0.f } },
        { { +s, -s, -s }, { 0.f, -1.f,  0.f } },
        { { +s, -s, +s }, { 0.f, -1.f,  0.f } },
        { { -s, -s, +s }, { 0.f, -1.f,  0.f } },
    };

    uint16_t indices[] = {
         0,  1,  2,  2,  3,  0, /* front */
         4,  5,  6,  6,  7,  4, /* right */
         8,  9, 10, 10, 11,  8, /* back */
        12, 13, 14, 14, 15, 12, /* left */
        16, 17, 18, 18, 19, 16, /* top */
        20, 21, 22, 22, 23, 20, /* bottom */
    };

    const size_t vertexDataSize = sizeof( verts );
    const size_t indexDataSize = sizeof( indices );

    vertexDataBuffer = ns_ptr(device->newBuffer( vertexDataSize, MTL::ResourceStorageModeManaged ));
    indexBuffer = ns_ptr(device->newBuffer( indexDataSize, MTL::ResourceStorageModeManaged ));

    memcpy( vertexDataBuffer->contents(), verts, vertexDataSize );
    memcpy( indexBuffer->contents(), indices, indexDataSize );

    vertexDataBuffer->didModifyRange( NS::Range::Make( 0, vertexDataBuffer->length() ) );
    indexBuffer->didModifyRange( NS::Range::Make( 0, indexBuffer->length() ) );

    const size_t instanceDataSize = kMaxFramesInFlight * kNumInstances * sizeof( InstanceData );
    for ( size_t i = 0; i < kMaxFramesInFlight; ++i ) {
        instanceDataBuffers[ i ] = ns_ptr(device->newBuffer( instanceDataSize, MTL::ResourceStorageModeManaged ));
    }

    const size_t cameraDataSize = kMaxFramesInFlight * sizeof( CameraData );
    for ( size_t i = 0; i < kMaxFramesInFlight; ++i ) {
        cameraDataBuffers[ i ] = ns_ptr(device->newBuffer( cameraDataSize, MTL::ResourceStorageModeManaged ));
    }
}

Renderer::Renderer(MTK::View *mtkView, Scene& scene)
: scene(scene)
, angle(0.f)
, frame(0) {
    mtkView->setColorPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB );
    mtkView->setClearColor( MTL::ClearColor::Make( 0.1, 0.1, 0.1, 1.0 ) );
    mtkView->setDepthStencilPixelFormat( MTL::PixelFormat::PixelFormatDepth16Unorm );
    mtkView->setClearDepth( 1.0f );
    
    device = retain(mtkView->device());
    
    // Load all the shader files with a .metal file extension in the project.
    buildShaders();
    buildDepthStencilStates();
    buildBuffers();

    q = ns_ptr(device->newCommandQueue());
    
    semaphore = dispatch_semaphore_create(Renderer::kMaxFramesInFlight);
}

Renderer::~Renderer() {}

void Renderer::drawInMTKView(MTK::View* view) {
    auto pool = NS::AutoreleasePool::alloc()->init();
    auto renderPassDesc = view->currentRenderPassDescriptor();
    
    if (renderPassDesc != nullptr) {
        auto cmdBuffer = q->commandBuffer();
        cmdBuffer->setLabel(NSExt::UTF8String("MyCommand"));
        dispatch_semaphore_wait( semaphore, DISPATCH_TIME_FOREVER );
        Renderer* renderer = this;
        cmdBuffer->addCompletedHandler( ^void( MTL::CommandBuffer* pCmd ){
            dispatch_semaphore_signal( renderer->semaphore );
        });
        auto enc = cmdBuffer->renderCommandEncoder(renderPassDesc);
        enc->setLabel(NSExt::UTF8String("MyRenderEncoder"));
        enc->setViewport(MTL::Viewport{
            .width=(double)viewport.x,
            .height=(double)viewport.y,
            .zfar=1.0
        });
        
        enc->setRenderPipelineState(state.get());
        
        using simd::float4;
        using simd::float4x4;
        using simd::float3;
        angle += 0.002f;

        // Update instance positions:
        frame = (frame + 1) % Renderer::kMaxFramesInFlight;
        auto instanceDataBuffer = instanceDataBuffers[ frame ];

        const float scl = 0.2f;
        InstanceData* pInstanceData = reinterpret_cast< InstanceData *>( instanceDataBuffer->contents() );

        float3 objectPosition = { 0.f, 0.f, -10.f };

//        float4x4 rt = Math::makeTranslate( objectPosition );
//        float4x4 rr1 = Math::makeYRotate( -angle );
//        float4x4 rr0 = Math::makeXRotate( angle * 0.5 );
//        float4x4 rtInv = Math::makeTranslate( { -objectPosition.x, -objectPosition.y, -objectPosition.z } );
//        float4x4 fullObjectRot = rt * rr1 * rr0 * rtInv;

        size_t ix = 0;
        size_t iy = 0;
        size_t iz = 0;
        for ( size_t i = 0; i < kNumInstances; ++i )
        {
            if ( ix == kInstanceRows )
            {
                ix = 0;
                iy += 1;
            }
            if ( iy == kInstanceRows )
            {
                iy = 0;
                iz += 1;
            }

            float4x4 scale = Math::makeScale( (float3){ scl, scl, scl } );
            float4x4 zrot = Math::makeZRotate( angle * sinf((float)ix) );
            float4x4 yrot = Math::makeYRotate( angle * cosf((float)iy));

            float x = ((float)ix - (float)kInstanceRows/2.f) * (2.f * scl) + scl;
            float y = ((float)iy - (float)kInstanceColumns/2.f) * (2.f * scl) + scl;
            float z = ((float)iz - (float)kInstanceDepth/2.f) * (2.f * scl);
            float4x4 translate = Math::makeTranslate( Math::add( objectPosition, { x, y, z } ) );

            //pInstanceData[ i ].instanceTransform = fullObjectRot * translate * yrot * zrot * scale;
            pInstanceData[ i ].instanceTransform = translate * yrot * zrot * scale;
            pInstanceData[ i ].instanceNormalTransform = Math::discardTranslation( pInstanceData[ i ].instanceTransform );

            float iDivNumInstances = i / (float)kNumInstances;
            float r = iDivNumInstances;
            float g = 1.0f - r;
            float b = sinf( M_PI * 2.0f * iDivNumInstances );
            pInstanceData[ i ].instanceColor = (float4){ r, g, b, 1.0f };

            ix += 1;
        }
        instanceDataBuffer->didModifyRange( NS::Range::Make( 0, instanceDataBuffer->length() ) );

        // Update camera state:

        auto pCameraDataBuffer = cameraDataBuffers[ frame ];
        CameraData* pCameraData = reinterpret_cast< CameraData *>( pCameraDataBuffer->contents() );
        pCameraData->perspectiveTransform = Math::makePerspective( 45.f * M_PI / 180.f, 1.f, 0.03f, 500.0f ) ;
        pCameraData->worldTransform = Math::makeIdentity();
        pCameraData->worldNormalTransform = Math::discardTranslation( pCameraData->worldTransform );
        pCameraDataBuffer->didModifyRange( NS::Range::Make( 0, sizeof( CameraData ) ) );

        enc->setRenderPipelineState( state.get() );
        enc->setDepthStencilState( depthStencilState.get() );

        enc->setVertexBuffer( vertexDataBuffer.get(), /* offset */ 0, /* index */ 0 );
        enc->setVertexBuffer( instanceDataBuffer.get(), /* offset */ 0, /* index */ 1 );
        enc->setVertexBuffer( pCameraDataBuffer.get(), /* offset */ 0, /* index */ 2 );

        enc->setCullMode( MTL::CullModeBack );
        enc->setFrontFacingWinding( MTL::Winding::WindingCounterClockwise );

        enc->drawIndexedPrimitives( MTL::PrimitiveType::PrimitiveTypeTriangle,
                                    6 * 6, MTL::IndexType::IndexTypeUInt16,
                                    indexBuffer.get(),
                                    0,
                                    kNumInstances );

        enc->endEncoding();
        cmdBuffer->presentDrawable(view->currentDrawable());
        cmdBuffer->commit();
    }
    pool->release();
}

void Renderer::drawableSizeWillChange(MTK::View* view, CGSize size) {
    viewport.x = size.width;
    viewport.y = size.height;
}

} /* namespace NavigateCube */
} /* namespace Scenes */

