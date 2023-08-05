#include <vector>
#include <array>
#include <type_traits>
#include <simd/simd.h>

#include "../../Utility/AppKitExt.hh"

#include "../../Engine/ShaderTypes.h"
#include "../../Engine/NativeRenderer.hh"
#include "../../Engine/Input.hh"
#include "./Scene.hh"

namespace Scenes {
/*
 Create a program called "Angry Birds 2D" which features a red and a green
 bird, along with a light green ground and a two-pronged slingshot sticking out
 of the ground in any color, with a wide black rubber band surrounding the red
 bird to be launched. The slingshot compartment, where the red bird awaits its
 fate, is located 200 pixels to the right and 400 pixels below the top-left
 corner of the application's window with a resolution of 600x600.

 The bodies, eyes, and eyeballs of the birds are elliptical (not circular),
 while their beaks, tail feathers, and eyebrows are triangles. Wings and tufts
 are optional. The green bird flies up and down even without wings, with its
 height changing sinusoidally in time until it hits an obstacle. The red bird
 can be launched from the slingshot according to the laws of physics, taking on
 the spring energy of the slingshot entirely at its launch. The launch process
 begins by pressing the left button of the mouse, which makes the bird stick to
 the inside of its elliptical-shaped body, and it follows the cursor's movement
 until the mouse button is released. The rubber band of the slingshot tightly
 wraps around the bird's bottom. Releasing the left button launches the red
 bird, which flies out of the slingshot. No worries about momentum or rotation.
 The red bird is subject to gravity during flight, but there is no air
 resistance or lift force since it doesn't have wings to flap. If the elliptical
 bodies of the red and green birds collide (note: the collision of two ellipses
 must be accurately calculated), both birds hover in the air, and the red bird
 turns yellow. If the red bird goes out of the visible range, a new one is
 automatically born in the slingshot, upside down between the two branches.

 Deadline: October 1, 2013, 23:59.
 */
namespace S13E01 {

namespace Colors {
constexpr auto black = simd::float3{};
constexpr auto white = simd::float3{1.0f,1.0f,1.0f};
constexpr auto yellow = simd::float3{1.0f,0.8f,0.0f};
} /* namespace colors */

constexpr simd::float2 scaleToViewport(simd::float2 norm) {
    return norm * 300;
}

constexpr double dt = 1.0 / 90;

const simd::float2 viewport{600, 600};

const simd::float3 skyColor{0.7f, 0.7f, 0.9f};
const std::array<simd::float2, 4> skyVertices{{
    {0, 0},
    {viewport.x, 0},
    {0, viewport.y},
    viewport,
}};

const simd::float3 grassColor{0.1f, 1.0f, 0.1f};
const std::array<simd::float2, 4> grassVertices{{
    {0, 0},
    {600, 0},
    {0, 60},
    {600, 60},
}};

const simd::float3 slingshotColor{0.76f,0.043f,0.043f};
const std::array<simd::float2, 6> slingshotBackVertices{{
    {198, 0},
    {207, 0},
    {198, 120},
    {207, 120},
    {222, 270},
    {231, 270},
}};
const std::array<simd::float2, 4> slingshotFrontVertices{{
    {198, 120},
    {207, 120},
    {174, 270},
    {183, 270},
}};
const simd::float2 launchPosition{200, 200};
const simd::float2 gravity{0.0f,-60.0f};

template <size_t N>
void drawPrimitive(MTL::RenderCommandEncoder* enc,
                   const std::array<simd::float2, N>& vertices,
                   const simd::float3& color,
                   MTL::PrimitiveType primitiveType = MTL::PrimitiveType::PrimitiveTypeTriangleStrip
                   ) {
    enc->setVertexBytes(vertices.data(), sizeof(vertices), VertexInputIndexVertices);
    enc->setVertexBytes(&color, sizeof(color), VertexInputIndexColor);
    enc->drawPrimitives(primitiveType, NS::UInteger(0), NS::UInteger(vertices.size()));
}

template <size_t N>
void drawEllipse(MTL::RenderCommandEncoder* enc,
                 const simd::float2& p,
                 const simd::float2& position,
                 const simd::float3& color
                 ) {
    std::array<simd::float2, 2 * N + 1> vertices;
    const float angleIncrement = 2.0f * M_PI / N;
    for (auto i = 0; i < N; ++i) {
        float angle = i * angleIncrement;
        vertices[2 * i] = {
            position.x + cosf(angle) * p.x,
            position.y + sinf(angle) * p.y,
        };
        vertices[2 * i + 1] = { position.x, position.y };
    }
    vertices[2 * N] = { position.x + p.x, position.y};
    drawPrimitive(enc, vertices, color);
}

enum class State{
    Idle,
    Dragging,
    Launching,
    Air,
    Hit
};

namespace Facing {
constexpr simd::float2 Left = {-1, 1};
constexpr simd::float2 Right = {1, 1};
constexpr simd::float2 UpsideDown = {1, -1};
} /* namespace Facing */

struct Bird {
    static constexpr size_t BodyResolution = 30;
    static constexpr simd::float2 p = {30.f, 60.f};
    
    simd::float2 position;
    simd::float3 color;
    simd::float2 facing;
    
    void draw(MTL::RenderCommandEncoder* enc) const;
    bool intersect(const Bird& anotherBird) const;
    bool intersect(const simd::float2& vertex) const;
};

void Bird::draw(MTL::RenderCommandEncoder* enc) const {
    // tail
    drawPrimitive(enc, std::array<simd::float2, 6> {
        simd::float2{position + p * simd::float2{-0.7f, 0.0f} * facing},
        simd::float2{position + p * simd::float2{-1.2f, 0.8f} * facing},
        simd::float2{position + p * simd::float2{-1.2f, 0.15f} * facing},
        simd::float2{position + p * simd::float2{-0.7f, 0.0f} * facing},
        simd::float2{position + p * simd::float2{-1.2f, 0.8f} * facing},
        simd::float2{position + p * simd::float2{-1.2f, -0.15f} * facing},
    }, Colors::black, MTL::PrimitiveType::PrimitiveTypeTriangle);
    
    drawEllipse<Bird::BodyResolution>(enc, p, position, color);
    
    // eyes
    drawEllipse<Bird::BodyResolution>(enc, p * 0.4, position + p * 0.5 * facing, Colors::white);
    drawEllipse<Bird::BodyResolution>(enc, p * 0.4, position + p * simd::float2{-0.1f, 0.5f} * facing, Colors::white);
    drawEllipse<Bird::BodyResolution>(enc, p * 0.2, position + p * 0.6 * facing, Colors::black);
    drawEllipse<Bird::BodyResolution>(enc, p * 0.2, position + p * simd::float2{0.0f, 0.6f} * facing, Colors::black);
    
    // beak
    drawPrimitive(enc, std::array<simd::float2, 3> {
        simd::float2{position + p * simd::float2{0.2f, 0.1f} * facing},
        simd::float2{position + p * simd::float2{0.2f, -0.1f} * facing},
        simd::float2{position + p * simd::float2{1.f, 0.0f} * facing},
    }, Colors::yellow);
    
    // eyebrows
    drawPrimitive(enc, std::array<simd::float2, 6> {
        simd::float2{position + p * simd::float2{0.6f, 0.8f} * facing},
        simd::float2{position + p * simd::float2{1.4f, 0.6f} * facing},
        simd::float2{position + p * simd::float2{0.6f, 1.0f} * facing},
        simd::float2{position + p * simd::float2{-0.1f, 1.0f} * facing},
        simd::float2{position + p * simd::float2{-0.9f, 0.6f} * facing},
        simd::float2{position + p * simd::float2{-0.1f, 0.8f} * facing},
    }, Colors::black, MTL::PrimitiveType::PrimitiveTypeTriangle);
}

bool Bird::intersect(const Bird& anotherBird) const {
    float d = (position.x - anotherBird.position.x) * (position.x - anotherBird.position.x) / (p.x * p.x) +
    (position.y - anotherBird.position.y) * (position.y - anotherBird.position.y) / (p.y * p.y);
    return d <= 4;
}

bool Bird::intersect(const simd::float2& vertex) const {
    float d = (position.x - vertex.x) * (position.x - vertex.x) / (p.x * p.x) +
    (position.y - vertex.y) * (position.y - vertex.y) / (p.y * p.y);
    return d <= 1;
}

State state(State::Idle);
simd::float2 launchVelocity;
simd::float2 velocity;
simd::float2 grabOffset;
CFTimeInterval startT;
CFTimeInterval launchT;

Bird target{simd::float2{525,300}, simd::float3{0,0.5f,0}, Facing::Left};
Bird missile{launchPosition, simd::float3{0.5f,0,0}, Facing::Right};

void onInit(CFTimeInterval t) {
    startT = t;
}

void onMouseClicked(Engine::Input::ButtonState buttonState, simd::float2 c) {
    if (state == State::Idle && buttonState == Engine::Input::ButtonState::Down && missile.intersect(c)) {
        state = State::Dragging;
        grabOffset = missile.position - c;
        return;
    }
    
    if (state == State::Dragging && buttonState == Engine::Input::ButtonState::Up) {
        launchVelocity = (launchPosition - missile.position) * 7.0f * dt;
        state = State::Launching;
        return;
    }
}

void onMouseMoved(simd::float2 c) {
    if(state == State::Dragging) {
        missile.position = c + grabOffset;
    }
}

void onIdle(CFTimeInterval endT) {
    auto t = startT;
    for(; t < endT; t += dt) {
        if (state != State::Hit) {
            target.position = simd::float2{525,300} + simd::float2{0.0f, sinf(2.0f * M_PI * t * 0.5f) * 210};
        }
        
 
        if(state == State::Launching){
            missile.position += launchVelocity;
            if(simd::distance(missile.position, launchPosition) < 20.0f){
                launchT = t;
                velocity = launchVelocity;
                state = State::Air;
            }
        }
 
        if(state == State::Air){
            missile.position += velocity;
            velocity = velocity + gravity * dt;

            if(missile.intersect(target)) {
                missile.color = simd::float3{1.0f,1.0f,0.0f};
                state = State::Hit;
                break;
            }
            if(missile.position.x > 660 || missile.position.x <= -60 || missile.position.y < -120 ) {
                state = State::Idle;
                missile = Bird{launchPosition, simd::float3{0.5f,0,0}, Facing::UpsideDown};
                break;
            }
        }
    }
    startT = t;
}

void onDraw(MTL::RenderCommandEncoder* enc) {
    simd::float2 center(launchPosition);
    if ( state == State::Idle || state == State::Dragging || state == State::Launching ) {
        center=missile.position;
    }
    
    enc->setVertexBytes(&viewport, sizeof(viewport), VertexInputIndexViewportSize);
    
    drawPrimitive(enc, skyVertices, skyColor);
    drawPrimitive(enc, grassVertices, grassColor);
    drawPrimitive(enc, slingshotBackVertices, slingshotColor);
    
    // rubber back
    drawPrimitive(enc, std::array<simd::float2, 6> {
        scaleToViewport(simd::float2{-0.27f,-0.333f} + 1),
        center + scaleToViewport(simd::float2{-0.1f, 0.04f}),
        center + scaleToViewport(simd::float2{-0.1f, -0.04f}),
        scaleToViewport(simd::float2{-0.27f,-0.333f} + 1),
        scaleToViewport(simd::float2{-0.27f,-0.300f} + 1),
        center + scaleToViewport(simd::float2{-0.1f, 0.04f})
    }, Colors::black, MTL::PrimitiveType::PrimitiveTypeTriangle);
    
    
    missile.draw(enc);
    target.draw(enc);
    
    drawPrimitive(enc, slingshotFrontVertices, slingshotColor);
    
    // rubber front
    drawPrimitive(enc, std::array<simd::float2, 6> {
        scaleToViewport(simd::float2{-0.333f,-0.333f} + 1),
        center + scaleToViewport(simd::float2{-0.1f, 0.04f}),
        center + scaleToViewport(simd::float2{-0.1f, -0.04f}),
        scaleToViewport(simd::float2{-0.333f,-0.333f} + 1),
        scaleToViewport(simd::float2{-0.333f,-0.300f} + 1),
        center + scaleToViewport(simd::float2{-0.1f, 0.04f})
    }, Colors::black, MTL::PrimitiveType::PrimitiveTypeTriangle);
}
} /* namespace S13E01 */
} /* namespace Scenes */
