#include <AppKit/AppKit.hpp>
#include "AppKitExt.hh"
#include <simd/simd.h>

#include "../../Engine/Engine.hh"

#include "./Scene.hh"

namespace Scenes {
namespace S13E02 {

Renderer::Renderer(NSExt::ScopedRef<MTL::Device> device, NSExt::ScopedRef<MTL::CommandQueue> q, NSExt::ScopedRef<MTL::RenderPipelineState> state, Scene& scene)
: device(device), q(q), state(state), scene(scene) {}

Renderer::~Renderer() {}

Renderer* Renderer::createRenderer(MTK::View* mtkView, Scene& scene) {
    auto device = NSExt::RetainScoped(mtkView->device());
    NS::Error* err = nullptr;
    
    mtkView->setColorPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB );
    mtkView->setClearColor( MTL::ClearColor::Make( 1.0, 1.0, 1.0, 1.0 ) );
    
    // Load all the shader files with a .metal file extension in the project.
    auto library = NSExt::MakeScoped(device->newDefaultLibrary());
    
    if ( !library ) {
        __builtin_printf( "%s", err->localizedDescription()->utf8String() );
        assert( false );
    }
    
    auto vertexShader = NSExt::MakeScoped(library->newFunction(NSExt::UTF8String("Scenes::S13E02::vertexShader")));
    auto fragmentShader = NSExt::MakeScoped(library->newFunction(NSExt::UTF8String("Scenes::S13E02::fragmentShader")));
    
    auto desc = NSExt::MakeScoped(MTL::RenderPipelineDescriptor::alloc()->init());
    
    desc->setVertexFunction(*vertexShader);
    desc->setFragmentFunction(*fragmentShader);
    desc->colorAttachments()->object(0)->setPixelFormat(mtkView->colorPixelFormat());

    auto state = NSExt::MakeScoped(device->newRenderPipelineState(*desc, &err));
    if ( !state )
    {
        __builtin_printf( "%s", err->localizedDescription()->utf8String() );
        assert( false );
    }

    auto q = NSExt::MakeScoped(device->newCommandQueue());
    return new Renderer(device, q, state, scene);
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
        
        enc->setRenderPipelineState(*state);
        
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
