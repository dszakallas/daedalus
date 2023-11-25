#pragma once

#include <CoreGraphics/CoreGraphics.h>
#include <QuartzCore/QuartzCore.h>
#include <MetalKit/MetalKit.hpp>
#include "../../Utility/AppKitExt.hh"
#include "../../Engine/Engine.hh"
#include "../../Engine/Input.hh"

namespace Scenes {
namespace S13E02 {

struct Scene : public Engine::Scene {
    Engine::Renderer* createRenderer(MTK::View *mtkView) override;
    void onIdle(CFTimeInterval time) override;
    void onInit(CFTimeInterval time) override;
    void onMouseClicked(Engine::Input::MouseButton button, Engine::Input::ButtonState buttonState, simd::float2 c) override;
    void onMouseMoved(simd::float2 c) override;
    bool onKey(Engine::Input::KeyboardButton button, Engine::Input::ButtonState state) override;
    ~Scene() override {};
    
    void onDraw(MTL::RenderCommandEncoder* enc);
};

struct Renderer : public Engine::Renderer {
    Renderer(NSExt::ScopedRef<MTL::Device> device, NSExt::ScopedRef<MTL::CommandQueue> q, NSExt::ScopedRef<MTL::RenderPipelineState> state, Scene& scene);
    static Renderer* createRenderer(MTK::View* mtkView, Scene& scene);
    virtual void drawInMTKView(MTK::View* view) override;
    virtual void drawableSizeWillChange(MTK::View* view, CGSize size) override;
    virtual ~Renderer() override;
private:
    NSExt::ScopedRef<MTL::Device> device;
    NSExt::ScopedRef<MTL::CommandQueue> q;
    NSExt::ScopedRef<MTL::RenderPipelineState> state;
    simd_uint2 viewport;
    Scene& scene;
};

} /* namespace S13E02 */
} /* namespace Scenes */
