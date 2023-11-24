#include <simd/simd.h>
#include <numbers>

#include "Scene.hh"
#include "ShaderTypes.hh"

namespace Scenes {

namespace S13E02 {

/*
 Create a 2D editor that allows arranging a 100m x 100m area, with a camera window of 50m x 50m.
 The window will be moved by a vector (10, 20) with each press of the 's' key. If the window
 reaches the edge of the area, it will wrap around to the bottom left corner. The program will
 treat the left mouse button presses as control points and the current time as parameter values.
 This means that each left mouse button press will add a new control point (up to a maximum of
 10 control points). The program will fit a white Tensioned Catmull-Rom (TCR) spline to the
 control points, with a tension of -0.5. The control points will be represented by red filled
 circles with a radius of 1m in their resting state.
 
 Pressing the space key will activate the program, duplicating the spline and rotating the new
 version by 60 degrees around the centroid of the control points (which can be calculated as the
 arithmetic mean of the control point coordinates). The program will then create a blue Bezier
 curve from the duplicated spline (while keeping the original TCR spline in place). Additionally,
 a yellow filled circle with a radius of 1m will start moving along each curve, following the
 parameterization of the TCR spline. This means that each circle will reach the end of the curve
 in the same amount of time it took to add the control point. The Bezier curve will be traversed
 by its own circle, arriving at the end at the same time as the TCR spline circle. The races start
 again. During the races, the radius of the filled circles representing the control points will
 continuously change, indicating the absolute value of the weight that should be assigned to each
 control point in order to obtain the current point on the curve as a combination of control points
 with these weights (for Bezier, think about the Bernstein polynomial; for TCR, some additional
 thinking may be required). If the weight is positive, the circle will be red. If it is negative,
 it will be turquoise blue.
 
 Note: Only approximately C2 continuous and exactly C1 continuous real, non-uniform TCR splines
 are acceptable. For example, Catmull-Rom splines and Kochanek-Bartels splines that include
 unnecessary elements are not allowed. To put it more simply, splines copied from the Internet
 are not recommended.
 */

namespace Colors {
constexpr simd::float3 black = {};
constexpr simd::float3 red = {1.0f};
constexpr simd::float3 blue = {0,0,1.0f};
constexpr simd::float3 yellow = {1.0f,1.0f,0.0f};
//constexpr auto white = simd::float3{1.0f,1.0f,1.0f};

} /* namespace colors */

template <size_t N>
INLINE
void drawPrimitive(MTL::RenderCommandEncoder* enc,
                   const std::array<simd::float2, N>& vertices,
                   const simd::float3& color,
                   MTL::PrimitiveType primitiveType = MTL::PrimitiveType::PrimitiveTypeTriangleStrip
                   ) {
    enc->setVertexBytes(vertices.data(), sizeof(vertices), (NS::UInteger)VertexInputIndex::Vertices);
    enc->setVertexBytes(&color, sizeof(color), (NS::UInteger)VertexInputIndex::Color);
    enc->drawPrimitives(primitiveType, NS::UInteger(0), NS::UInteger(vertices.size()));
}

template <size_t N>
INLINE
void drawEllipse(MTL::RenderCommandEncoder* enc,
                 const simd::float2& p,
                 const simd::float2& position,
                 const simd::float3& color
                 ) {
    std::array<simd::float2, 2 * N + 1> vertices;
    auto angleIncrement = 2.0f * std::numbers::pi_v<float> / N;
    for (auto i = 0; i < N; ++i) {
        auto angle = i * angleIncrement;
        vertices[2 * i] = {
            position.x + cosf(angle) * p.x,
            position.y + sinf(angle) * p.y,
        };
        vertices[2 * i + 1] = { position.x, position.y };
    }
    vertices[2 * N] = { position.x + p.x, position.y};
    drawPrimitive(enc, vertices, color);
}

template <size_t N>
void drawCircle(MTL::RenderCommandEncoder* enc,
                float p,
                const simd::float2& position,
                const simd::float3& color
                ) {
    drawEllipse<N>(enc, simd::float2{p, p}, position, color);
}

enum class PresentationStateTag: size_t { Edit, Animation };

struct EditStateVars { };
struct AnimationStateVars { float dt; };
union PresentationStateVars {
    EditStateVars edit;
    AnimationStateVars anim;
};

struct PresentationState {
    PresentationStateTag tag;
    CFTimeInterval enteredT;
    PresentationStateVars vars;
};

template<class _T>
struct object {
    _T data;
    simd::float4x4 obj;
};

const simd::float2 viewport{600, 600};
const simd::float4x4 clip = simd_float4x4{{
    {0.02, 0, 0, 0},
    {0, 0.02, 0, 0},
    {0, 0, 1.0, 0},
    {-1, -1, 0, 1.0}
}};
const simd::float4x4 defaultCam = simd_float4x4{{
    {2, 0, 0, 0},
    {0, 2, 0, 0},
    {0, 0, 1.0, 0},
    {0, 0, 0, 1.0}
}};

struct TCR {
    static constexpr simd::float4 tcr(
                                      std::array<simd::float4, 2> p,
                                      std::array<simd::float4, 2> v,
                                      std::array<float, 2> t,
                                      float tx
                                      ) {
        const auto a0 = p[0];
        const auto a1 = v[0];
        
        const float tau = t[1] - t[0];
        const simd::float4 eps = p[1] - p[0];
        
        const simd::float4 a2 = (eps * 3.0f) / (tau * tau) - (v[1] + v[0] * 2.0f) / tau;
        const simd::float4 a3 = (eps * -2.0f) / (tau * tau * tau) + (v[1] + v[0]) / (tau * tau);
        
        const float dt = tx - t[0];
        return a0 + a1 * dt + a2 * dt * dt + a3 * dt * dt * dt;
    }
    static constexpr float tension = -0.5f;
    static constexpr size_t N = 10;
    
    int count;
    float t[N];
    simd::float4 p[N];
    simd::float4 v[N];
    
    int index(float t) const {
        for(auto i = 0; i < count; ++i) {
            if(this->t[i] > t) return i-1;
        }
        return 0;
    }
    
    void addControlPoint(const simd::float4& px, float tx) {
        if(count == N) return;
        
        p[count] = px;
        t[count] = tx;
        count++;
        if(count > 2) {
            v[count-2] = velocity(count-2);
        }
        
        return;
    }
    
    simd::float4 velocity(int i) const {
        if (i < 0 || i >= count - 1) {
            return {};
        }
        
        simd::float4 v{};
        for (auto j = i; j <= i+1; ++j) {
            v = (p[i] - p[i-1]) / (t[i] - t[i-1]);
        }
        
        return v * ((1.0f - tension) / 2.0f);
    }
    
    simd::float4 weight(float tx) {
        if (count == 0) {
            return {};
        }
        
        simd::float4 p_[4]{
            {1,},
            {0,1},
            {0,0,1},
            {0,0,0,1}
        };
        
        if (count == 1 || tx <= t[0]) {
            return p_[1];
        }
        
        if (tx >= t[count-1]) {
            return p_[2];
        }
        
        int i = index(tx);
        
        auto v_ = std::array<simd::float4, 2>{};
        for (auto j = i; j < i+2; ++j) {
            v_[j-i] = (i-1 < 0 || i+1 > count) ? simd::float4{} :
            ((p_[j-i+1] - p_[j-i])/(t[i] - t[i-1]) + (p_[j-i+2] - p_[j-i+1])/(t[i+1] - t[i])) *
            ((1.0f - tension) / 2.0f);
        }
        
        return tcr(
                   std::array<simd::float4, 2>{p_[1], p_[2]},
                   v_,
                   std::array<float, 2>{t[i+1], t[i]},
                   tx
                   );
    }
    
    simd::float4 operator()(float tx) {
        if (count == 0) {
            return {};
        }
        
        if (count == 1 || tx <= t[0]) {
            return p[0];
        }
        
        if (tx >= t[count-1]) {
            return p[count-1];
        }
        
        int i = index(tx);
        
        return tcr(
                   std::array<simd::float4, 2>{p[i], p[i+1]},
                   std::array<simd::float4, 2>{v[i], v[i+1]},
                   std::array<float, 2>{t[i], t[i+1]},
                   tx
                   );
    }
    
    void onDraw(MTL::RenderCommandEncoder* enc, const PresentationState& state) {
        if (count == 0) {
            return;
        }
        const int res = 500;
        auto ts = t[0];
        auto te = t[count - 1];
        
        std::array<simd::float2, res> vertices{};
        for (auto i = 0; i < res; ++i) {
            auto p = (*this)((te - ts) * ((float) i / res) + ts);
            vertices[i] = p.xy;
        }
        
        drawPrimitive(enc, vertices, Colors::black, MTL::PrimitiveTypeLineStrip);
        
        if (state.tag == PresentationStateTag::Edit) {
            for (auto i = 0; i < count; ++i) {
                drawCircle<20>(enc, 1, p[i].xy, Colors::red);
            }
            return;
        }
        auto dt = state.vars.anim.dt;
        auto ct = (float)(CACurrentMediaTime() - state.enteredT);
        auto t1 = (ct / dt) - floorf(ct / dt);
        auto t_abs = t[0] + t1 * dt;
        auto w = weight(t_abs);
        auto i = index(t_abs);

        for(auto j = 0; j < 4; ++j ){
            int index = i + j - 1;
            if(index >= 0 && index < count) {
                auto color = w[j] < 0 ? simd::float3{0,1,1} : Colors::red;
                drawCircle<20>(enc, w[j], p[index].xy, color);
            }
        }

        auto r = (*this)(t_abs);
        drawCircle<20>(enc, 1, r.xy, Colors::yellow);
    }
};

struct Bezier {
    static constexpr size_t N = 10;
    
    int count;
    simd::float4 p[N];
    
    void addControlPoint(const simd::float4& px) {
        if(count == N) return;
        p[count] = px;
        count++;
        return;
    }
    float weight(int i, float t) const {
        float choose = 1;
        for(auto j = 1; j <= i; j++) {
            choose *= (float)(count - j) / j;
        }
        return choose * pow(t, i) * pow(1 - t, count - 1 - i);
    }
    simd::float4 operator()(float t) const {
        if(count == 0) {
            return {};
        }
        if(t < 0 || t > 1) {
            return p[0];
        }
        simd::float4 result{};
        for(int i = 0; i < count; i++) {
            result += p[i] * weight(i, t);
        }
        return result;
    }
    void onDraw(MTL::RenderCommandEncoder* enc, const PresentationState& state) {
        if (count == 0) {
            return;
        }
        constexpr size_t res = 500;
        std::array<simd::float2, res> vertices{};
        for (auto i = 0; i < res; ++i) {
            auto t = (float) i / res;
            auto p = (*this)(t);
            vertices[i] = p.xy;
        }
        drawPrimitive(enc, vertices, Colors::blue, MTL::PrimitiveTypeLineStrip);
        
//        float t_n = 0;
//        if (state.tag == PresentationStateTag::Animation) {
//            auto dt = state.vars.anim.dt;
//            auto ct = (float)(CACurrentMediaTime() - state.enteredT);
//            t_n = (ct / dt) - floorf(ct / dt);
//        }
        
        for(int i = 0; i < count; i++){
            auto w = 1; //weight(i, t_n);
            drawCircle<20>(enc, 1 * w, p[i].xy, Colors::red);
        }
    }
};

PresentationState state;
simd::float4x4 cam;
TCR tcr;
Bezier bezier;

void copyTCRToBezier() {
    for(int i = 0; i < tcr.count; ++i) {
        bezier.addControlPoint(tcr.p[i] - simd::float4{2, 2});
    }
}

void moveCamera() {
    auto tr = matrix_identity_float4x4;
    tr.columns[3] = simd::float4{-10, -20, 0, 1};
    cam = cam * tr;
    
    if (cam.columns[3].x < -100 || cam.columns[3].y < -100) {
        cam = defaultCam;
    }
}

void Scene::onDraw(MTL::RenderCommandEncoder* enc) {
    enc->setVertexBytes(&cam, sizeof(cam), (NS::UInteger)VertexInputIndex::Cam);
    enc->setVertexBytes(&clip, sizeof(clip), (NS::UInteger)VertexInputIndex::Clip);
    tcr.onDraw(enc, state);
    bezier.onDraw(enc, state);
}

void Scene::onInit(CFTimeInterval t) {
    state.enteredT = CACurrentMediaTime();
    cam = defaultCam;
    tcr = TCR{};
    bezier = Bezier{};
}

void Scene::onMouseClicked(Engine::Input::MouseButton button, Engine::Input::ButtonState buttonState, simd::float2 c) {
    if (button == Engine::Input::MouseButton::Left && buttonState == Engine::Input::ButtonState::Down &&
        state.tag == PresentationStateTag::Edit) {
        tcr.addControlPoint(simd::float4{
            (c.x / 6 - cam.columns[3][0]) / cam.columns[0][0],
            (c.y / 6 - cam.columns[3][1]) / cam.columns[1][1]
        }, CACurrentMediaTime() - state.enteredT);
    }
}

bool Scene::onKey(Engine::Input::KeyboardButton button, Engine::Input::ButtonState buttonState) {
    if (button == Engine::Input::KeyboardButton::SPACEBAR
        && state.tag == PresentationStateTag::Edit
        && tcr.count
        ) {
        state = PresentationState{
            .tag=PresentationStateTag::Animation,
            .enteredT=CACurrentMediaTime()
        };
        state.vars.anim = {tcr.t[tcr.count-1] - tcr.t[0]};
        copyTCRToBezier();
        return true;
    }
    if (button == Engine::Input::KeyboardButton::S) {
        moveCamera();
        return true;
    }
    return false;
}

void Scene::onMouseMoved(simd::float2 c) {
}

void Scene::onIdle(CFTimeInterval endT) {
}

Engine::Renderer* Scene::createRenderer(MTK::View* mtkView) {
    return new Renderer(mtkView, *this);
}

} /* namespace S13E02 */
} /* namespace Scenes */
