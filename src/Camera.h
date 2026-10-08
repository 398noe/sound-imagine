#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace imagine
{
struct Vec3 { float x, y, z; };
struct Camera
{
    // Unit quaternion maps world X=frequency, Y=Side, Z=RMS to camera space.
    float w = 1, x = 0, y = 0, z = 0, zoom = 1;
    float panX = 0, panY = 0;
    static Camera multiply(const Camera& a, const Camera& b)
    {
        Camera c;
        c.w = a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z;
        c.x = a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y;
        c.y = a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x;
        c.z = a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w;
        c.zoom = b.zoom; c.panX=b.panX; c.panY=b.panY; c.normalise(); return c;
    }
    void normalise()
    {
        const float n = std::sqrt(w*w+x*x+y*y+z*z);
        if (!std::isfinite(n) || n < 0.00001f) { w=1; x=y=z=0; }
        else { w/=n; x/=n; y/=n; z/=n; }
        zoom = std::isfinite(zoom) ? std::clamp(zoom,0.5f,2.5f) : 1;
        panX = std::isfinite(panX) ? std::clamp(panX,-2.f,2.f) : 0;
        panY = std::isfinite(panY) ? std::clamp(panY,-2.f,2.f) : 0;
    }
    Vec3 transform(Vec3 v) const
    {
        const Vec3 t { 2*(y*v.z-z*v.y), 2*(z*v.x-x*v.z), 2*(x*v.y-y*v.x) };
        return { v.x+w*t.x+y*t.z-z*t.y, v.y+w*t.y+z*t.x-x*t.z, v.z+w*t.z+x*t.y-y*t.x };
    }
    static Camera aligned(int axis)
    {
        Camera c;
        if (axis == 0) { c.w=0.5f; c.x=c.y=c.z=-0.5f; }
        if (axis == 1) { c.w=std::sqrt(0.5f); c.x=-c.w; }
        return c;
    }
    static Camera home()
    {
        Camera pitch, yaw;
        pitch.w=std::cos(-0.55f); pitch.x=std::sin(-0.55f);
        yaw.w=std::cos(-0.23f); yaw.z=std::sin(-0.23f);
        return multiply(pitch,yaw);
    }
    int alignedAxis() const
    {
        const std::array<Vec3,3> axes {{{1,0,0},{0,1,0},{0,0,1}}};
        for (int i=0;i<3;++i) if (std::abs(transform(axes[static_cast<size_t>(i)]).z)>0.99999f) return i;
        return -1;
    }
    Vec3 projectionScale(float width, float height) const
    {
        // Fit each display dimension independently, including the wide axis views.
        const auto a=transform({2,0,0}),b=transform({0,1.4f,0}),c=transform({0,0,1.4f});
        return {width*zoom/std::max(0.01f,std::abs(a.x)+std::abs(b.x)+std::abs(c.x)),
                height*zoom/std::max(0.01f,std::abs(a.y)+std::abs(b.y)+std::abs(c.y)),0};
    }
    void orbit(float fromX, float fromY, float toX, float toY)
    {
        // Displacement controls angles about fixed screen axes. No virtual
        // sphere, position-dependent roll, or limit on the number of turns.
        constexpr float pi=3.14159265358979323846f;
        const float yawAngle=(toX-fromX)*pi,pitchAngle=-(toY-fromY)*pi;
        Camera yaw,pitch;
        yaw.w=std::cos(yawAngle); yaw.y=std::sin(yawAngle);
        pitch.w=std::cos(pitchAngle); pitch.x=std::sin(pitchAngle);
        *this=multiply(pitch,multiply(yaw,*this));
    }
};
}
