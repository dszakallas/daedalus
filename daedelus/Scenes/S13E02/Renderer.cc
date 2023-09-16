#include <AppKit/AppKit.hpp>
#include "AppKitExt.hh"
#include <simd/simd.h>

#include "../../Engine/NativeRenderer.hh"
#include "../../Engine/ShaderTypes.h"

#include "./Scene.hh"

namespace Scenes {
namespace S13E02 {

Renderer::Renderer(MTK::View *mtkView, Scene& scene): scene(scene) {
    NS::Error* err = nullptr;
    
    mtkView->setColorPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB );
    mtkView->setClearColor( MTL::ClearColor::Make( 1.0, 1.0, 1.0, 1.0 ) );
    
    device = mtkView->device()->retain();
    
    // Load all the shader files with a .metal file extension in the project.
    MTL::Library* library = device->newDefaultLibrary();
    
//    NS::String* shaderSource = NSExt::StringWithContentsOfFile(
//                                                               NSExt::UTF8String("Shaders.metal"),
//                                                               NS::StringEncoding::UTF8StringEncoding,
//                                                               &err
//                                                               );
//    if ( !shaderSource )
//    {
//        __builtin_printf( "%s\n", err->localizedDescription()->utf8String() );
//        assert( false );
//    }
    
//    MTL::Library* library = device->newLibrary(shaderSource, nullptr, &err);
//
    //NSString *shaderSource = [NSString stringWithContentsOfFile:@"path_to_shader.metal" encoding:NSUTF8StringEncoding error:&error];
    //id<MTLLibrary> library = [device newLibraryWithSource:shaderSource options:nil error:&error];
//    if (!library) {
//        NSLog(@"Failed to create library: %@", error);
//    }
    
    if ( !library )
    {
        __builtin_printf( "%s", err->localizedDescription()->utf8String() );
        assert( false );
    }
    
    MTL::Function* vertexShader = library->newFunction(NSExt::UTF8String("Scenes::S13E02::vertexShader"));
    MTL::Function* fragmentShader = library->newFunction(NSExt::UTF8String("Scenes::S13E02::fragmentShader"));
    
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
