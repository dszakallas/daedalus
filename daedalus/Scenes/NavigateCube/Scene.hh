#pragma once

#include <CoreGraphics/CoreGraphics.h>
#include <QuartzCore/QuartzCore.h>
#include <MetalKit/MetalKit.hpp>
#include "../../Utility/AppKitExt.hh"
#include "../../Engine/Engine.hh"
#include "../../Engine/Input.hh"

namespace Scenes {
namespace NavigateCube {

using namespace NSExt;

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
    Renderer(MTK::View* mtkView, Scene& scene);
    virtual void drawInMTKView(MTK::View* view) override;
    virtual void drawableSizeWillChange(MTK::View* view, CGSize size) override;
    virtual ~Renderer() override;
    
    static constexpr size_t kMaxFramesInFlight = 3;
    static constexpr size_t kInstanceRows = 10;
    static constexpr size_t kInstanceColumns = 10;
    static constexpr size_t kInstanceDepth = 10;
    static constexpr size_t kNumInstances = (kInstanceRows * kInstanceColumns * kInstanceDepth);
private:
    void buildShaders();
    void buildDepthStencilStates();
    void buildBuffers();
    ns_ptr<MTL::Device> device;
    ns_ptr<MTL::CommandQueue> q;
    ns_ptr<MTL::RenderPipelineState> state;
    ns_ptr<MTL::Library> library;
    ns_ptr<MTL::DepthStencilState> depthStencilState;
    ns_ptr<MTL::Buffer> vertexDataBuffer;
    ns_ptr<MTL::Buffer> instanceDataBuffers[kMaxFramesInFlight];
    ns_ptr<MTL::Buffer> cameraDataBuffers[kMaxFramesInFlight];
    ns_ptr<MTL::Buffer> indexBuffer;
    float angle;
    int frame;
    dispatch_semaphore_t semaphore;
    simd_uint2 viewport;
    Scene& scene;
};

} /* namespace NavigateCube */
} /* namespace Scenes */
