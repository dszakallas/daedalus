#pragma once

#include <simd/simd.h>
#include <CoreGraphics/CoreGraphics.h>
#include <MetalKit/MetalKit.hpp>

#include "./Input.hh"

#define INLINE _MTL_INLINE

namespace Engine {

struct Renderer : public MTK::ViewDelegate {};

struct Scene {
    virtual Engine::Renderer*  createRenderer(MTK::View *mtkView) = 0;
    virtual void onIdle(CFTimeInterval time) = 0;
    virtual void onInit(CFTimeInterval time) = 0;
    virtual void onMouseClicked(Input::ButtonState buttonState, simd::float2 c) = 0;
    virtual void onMouseMoved(simd::float2 c) = 0;
    virtual ~Scene() {};
};

} /*namespace Engine */
