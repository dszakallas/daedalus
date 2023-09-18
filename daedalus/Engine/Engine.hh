#pragma once

#include <simd/simd.h>
#include <CoreGraphics/CoreGraphics.h>
#include <MetalKit/MetalKit.hpp>

#include "./Input.hh"

#define INLINE _MTL_INLINE

namespace Engine {

struct Renderer : public MTK::ViewDelegate {};

struct Scene {
    virtual Engine::Renderer* createRenderer(MTK::View *mtkView) = 0;
    virtual void onIdle(CFTimeInterval time) = 0;
    virtual void onInit(CFTimeInterval time) = 0;
    
    // Handle mouse click event. Coordinates are in pixels increasing from the
    // lower left corner to the top and to the right.
    virtual void onMouseClicked(Input::MouseButton button,
                                Input::ButtonState buttonState,
                                simd::float2 c
                                ) = 0;
    
    // Handle mouse move event. Coordinates are in pixels increasing from the
    // lower left corner to the top and to the right.
    virtual void onMouseMoved(simd::float2 c) = 0;
    
    // Handle physical keyboard event. Return boolean indicating whether the key was handled.
    virtual bool onKey(Engine::Input::KeyboardButton button, Engine::Input::ButtonState state) = 0;
    
    virtual ~Scene() {};
};

} /*namespace Engine */
