#pragma once

#include <CoreGraphics/CoreGraphics.h>
#include <QuartzCore/QuartzCore.h>
#include <MetalKit/MetalKit.hpp>
#include "../../Engine/NativeRenderer.hh"
#include "../../Engine/Input.hh"

namespace Scenes {
namespace S13E01 {

struct Renderer : public NativeRenderer {
    Renderer(MTK::View* mtkView);
    virtual void drawInMTKView(MTK::View* view) override;
    virtual void drawableSizeWillChange(MTK::View* view, CGSize size) override;
    virtual ~Renderer() override;
private:
    MTL::Device* device;
    MTL::CommandQueue* q;
    MTL::RenderPipelineState* state;
    simd_uint2 viewport;
};

void onDraw(MTL::RenderCommandEncoder* enc);

void onIdle(CFTimeInterval time);
void onInit(CFTimeInterval time);
void onMouseClicked(Engine::Input::ButtonState buttonState, simd::float2 c);
void onMouseMoved(simd::float2 c);

} /* namespace S13E01 */
} /* namespace Scenes */
