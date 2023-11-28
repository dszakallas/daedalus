#include <AppKit/AppKit.hpp>
#include "AppKitExt.hh"
#include <simd/simd.h>

#include "../../Engine/Engine.hh"

#include "./Scene.hh"

namespace Scenes {
namespace S13E02 {

using namespace NSExt;

Renderer::Renderer(MTK::View* mtkView, Scene& scene)
: scene(scene) {
    device = retain(mtkView->device());
    NS::Error* err = nullptr;
    
    mtkView->setColorPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB );
    mtkView->setClearColor( MTL::ClearColor::Make( 1.0, 1.0, 1.0, 1.0 ) );
    
    // Load all the shader files with a .metal file extension in the project.
    auto library = ns_ptr(device->newDefaultLibrary());
    
    if ( !library ) {
        __builtin_printf( "%s", err->localizedDescription()->utf8String() );
        assert( false );
    }
    
    auto vertexShader = ns_ptr(library->newFunction(NSExt::UTF8String("Scenes::S13E02::vertexShader")));
    auto fragmentShader = ns_ptr(library->newFunction(NSExt::UTF8String("Scenes::S13E02::fragmentShader")));
    
    auto desc = ns_ptr(MTL::RenderPipelineDescriptor::alloc()->init());
    
    desc->setVertexFunction(vertexShader.get());
    desc->setFragmentFunction(fragmentShader.get());
    desc->colorAttachments()->object(0)->setPixelFormat(mtkView->colorPixelFormat());

    state = ns_ptr(device->newRenderPipelineState(desc.get(), &err));
    if ( !state )
    {
        __builtin_printf( "%s", err->localizedDescription()->utf8String() );
        assert( false );
    }

    q = ns_ptr(device->newCommandQueue());
}

Renderer::~Renderer() {}

void Renderer::drawInMTKView(MTK::View* view) {
    auto pool = NS::AutoreleasePool::alloc()->init();
    
    auto cmdBuffer = q->commandBuffer();
    cmdBuffer->setLabel(UTF8String("MyCommand"));
    auto renderPassDesc = view->currentRenderPassDescriptor();
    
    if (renderPassDesc != nullptr) {
        auto enc = cmdBuffer->renderCommandEncoder(renderPassDesc);
        enc->setLabel(UTF8String("MyRenderEncoder"));
        enc->setViewport(MTL::Viewport{
            .width=(double)viewport.x,
            .height=(double)viewport.y,
            .zfar=1.0
        });
        
        enc->setRenderPipelineState(state.get());
        
        scene.onDraw(enc);
        
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

} /* namespace S13E02 */
} /* namespace Scenes */
