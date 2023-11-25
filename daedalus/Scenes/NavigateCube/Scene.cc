#include <simd/simd.h>
#include <numbers>

#include "../../Utility/Math.hh"
#include "Scene.hh"
#include "ShaderTypes.hh"

namespace Scenes {
namespace NavigateCube {

/*
 Mostly adapted from the Apple Metal C++ example code, adding
 joystick controls.
 */


void Scene::onDraw(MTL::RenderCommandEncoder* enc) {
}

void Scene::onInit(CFTimeInterval t) {
}

void Scene::onMouseClicked(Engine::Input::MouseButton button, Engine::Input::ButtonState buttonState, simd::float2 c) {
}

bool Scene::onKey(Engine::Input::KeyboardButton button, Engine::Input::ButtonState buttonState) {

    return false;
}

void Scene::onMouseMoved(simd::float2 c) {
}

void Scene::onIdle(CFTimeInterval endT) {
}

Engine::Renderer* Scene::createRenderer(MTK::View* mtkView) {
    return new Renderer(mtkView, *this);
}

} /* namespace NavigateCube */
} /* namespace Scenes */
