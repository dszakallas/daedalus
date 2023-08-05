#include <AppKit/AppKit.hpp>
#include "AppKitExt.hh"
#include <simd/simd.h>

#include "../../Engine/NativeRenderer.hh"
#include "../../Engine/ShaderTypes.h"

#include "./Scene.hh"

namespace Scenes {
namespace S13E01 {

Renderer::Renderer(MTK::View *mtkView) {
    NS::Error* err = nullptr;
    
    mtkView->setColorPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB );
    mtkView->setClearColor( MTL::ClearColor::Make( 0.1, 0.1, 0.1, 1.0 ) );
    
    device = mtkView->device()->retain();
    
    // Load all the shader files with a .metal file extension in the project.
    MTL::Library* library = device->newDefaultLibrary();
    if ( !library )
    {
        __builtin_printf( "%s", err->localizedDescription()->utf8String() );
        assert( false );
    }
    
    MTL::Function* vertexShader = library->newFunction(NSExt::UTF8String("vertexShader"));
    MTL::Function* fragmentShader = library->newFunction(NSExt::UTF8String("fragmentShader"));
    
    MTL::RenderPipelineDescriptor* desc = MTL::RenderPipelineDescriptor::alloc()->init();
    
    desc->setVertexFunction(vertexShader);
    desc->setFragmentFunction(fragmentShader);
    desc->colorAttachments()->object(0)->setPixelFormat(mtkView->colorPixelFormat());

    state = device->newRenderPipelineState(desc, &err);
    if ( !state )
    {
        __builtin_printf( "%s", err->localizedDescription()->utf8String() );
        assert( false );
    }

    q = device->newCommandQueue();
    
    vertexShader->release();
    fragmentShader->release();
    desc->release();
    library->release();
}

Renderer::~Renderer() {
    q->release();
    state->release();
    device->release();
}

void Renderer::drawInMTKView(MTK::View* view) {
    auto pool = NS::AutoreleasePool::alloc()->init();
    
    auto cmdBuffer = q->commandBuffer();
    cmdBuffer->setLabel(NSExt::UTF8String("MyCommand"));
    auto renderPassDesc = view->currentRenderPassDescriptor();
    
    if (renderPassDesc != nullptr) {
        auto enc = cmdBuffer->renderCommandEncoder(renderPassDesc);
        enc->setLabel(NSExt::UTF8String("MyRenderEncoder"));
        enc->setViewport(MTL::Viewport{
            .width=(double)viewport.x,
            .height=(double)viewport.y,
            .zfar=1.0
        });
        
        enc->setRenderPipelineState(state);
        
        onDraw(enc);
        
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

} /* namespace S13E01 */
} /* namespace Scenes */
