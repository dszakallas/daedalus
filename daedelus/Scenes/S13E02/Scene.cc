#include <simd/simd.h>

#include "Scene.hh"

#include "../../Engine/ShaderTypes.h"

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
constexpr auto black = simd::float3{};
constexpr auto red = simd::float3{1.0f};
constexpr auto blue = simd::float3{0,0,1.0f};
constexpr auto yellow = simd::float3{1.0f,1.0f,0.0f};
//constexpr auto white = simd::float3{1.0f,1.0f,1.0f};

} /* namespace colors */

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

template <size_t N>
void drawCircle(MTL::RenderCommandEncoder* enc,
                 float p,
                 const simd::float2& position,
                 const simd::float3& color
                 ) {
    drawEllipse<N>(enc, simd::float2{p, p}, position, color);
}

const simd::float2 viewport{600, 600};

enum class PresentationState {
    Edit,
    Animation,
};

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
    PresentationState state;

    int index(float t) const {
        for(int i = 0; i < count; i++) {
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
        
        const simd::float4 p_[4]{
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
    
    void onDraw(MTL::RenderCommandEncoder* enc) {
        if (count == 0) {
            return;
        }
        const int res = 500;
        const float ts = t[0];
        const float te = t[count - 1];
        
        std::array<simd::float2, res> vertices{};
        for (auto i = 0; i < res; ++i) {
            auto p = (*this)((te - ts) * ((float) i / res) + ts);
            vertices[i] = p.xy;
        }
        
        drawPrimitive(enc, vertices, Colors::black, MTL::PrimitiveTypeLineStrip);
        
        for (auto i = 0; i < count; ++i) {
            drawCircle<20>(enc, 10, p[i].xy, Colors::red);
        }
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
    void onDraw(MTL::RenderCommandEncoder* enc) {
        if (count == 0) {
            return;
        }
        constexpr size_t res = 500;
        std::array<simd::float2, res> vertices{};
        for (auto i = 0; i < res; ++i) {
            float t = (float) i / res;
            auto p = (*this)(t);
            vertices[i] = p.xy;
        }
        drawPrimitive(enc, vertices, Colors::blue, MTL::PrimitiveTypeLineStrip);
 
        //float ct = (float)(glutGet(GLUT_ELAPSED_TIME) - startTime) / 1000.0f;
        //float t_n = (ct / dt) - floorf(ct / dt);
 
        //glColor3f(1,0,0);
 
        for(int i = 0; i < count; i++){
            float w = 1; //weight(i, t_n);
            drawCircle<20>(enc, 10 * w, p[i].xy, Colors::red);
        }
 
        //glColor3f(1,1,0);
 
//        float4 r(this->operator()(t_n));
//        glColor3f(1,1,0);
//        glBegin(GL_POLYGON);
//        for(float t = 0; t < 1; t+=0.05f) {
//            float x = r.a + cosf(2 * t * M_PI);
//            float y = r.b + sinf(2 * t * M_PI);
//            glVertex2f(x,y);
//        }
//        glEnd();
    }
};

TCR tcr;
Bezier bezier;
PresentationState state;
CFTimeInterval startT;

void Scene::onDraw(MTL::RenderCommandEncoder* enc) {
    enc->setVertexBytes(&viewport, sizeof(viewport), VertexInputIndexViewportSize);
    tcr.onDraw(enc);
    bezier.onDraw(enc);
}

void Scene::onInit(CFTimeInterval t) {
    startT = CACurrentMediaTime();
    
    tcr = TCR{};
    bezier = Bezier{};
    bezier.addControlPoint({ 400, 400 });
    bezier.addControlPoint({ 400, 500 });
    bezier.addControlPoint({ 500, 500 });
    bezier.addControlPoint({ 500, 400 });
    bezier.addControlPoint({ 500, 300 });
}

void Scene::onMouseClicked(Engine::Input::ButtonState buttonState, simd::float2 c) {
    if (buttonState == Engine::Input::ButtonState::Down && state == PresentationState::Edit) {
        tcr.addControlPoint(simd::float4{c.x, c.y}, CACurrentMediaTime() - startT);
    }
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
