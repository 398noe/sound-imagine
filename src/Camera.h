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
    static Camera multiply(const Camera& a, const Camera& b)
    {
        Camera c;
        c.w = a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z;
        c.x = a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y;
        c.y = a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x;
        c.z = a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w;
        c.zoom = b.zoom; c.normalise(); return c;
    }
    void normalise()
    {
        const float n = std::sqrt(w*w+x*x+y*y+z*z);
        if (!std::isfinite(n) || n < 0.00001f) { w=1; x=y=z=0; }
        else { w/=n; x/=n; y/=n; z/=n; }
        zoom = std::isfinite(zoom) ? std::clamp(zoom,0.5f,2.5f) : 1;
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
    static Vec3 sphere(float px, float py)
    {
        const float r = px*px+py*py;
        if (r <= 1) return { px,py,std::sqrt(1-r) };
        const float n = std::sqrt(r); return { px/n,py/n,0 };
    }
    void orbit(float fromX, float fromY, float toX, float toY)
    {
        const auto a=sphere(fromX,fromY), b=sphere(toX,toY);
        Camera delta;
        delta.w=1+a.x*b.x+a.y*b.y+a.z*b.z;
        delta.x=a.y*b.z-a.z*b.y; delta.y=a.z*b.x-a.x*b.z; delta.z=a.x*b.y-a.y*b.x;
        if (delta.w < 0.00001f) // Antipodal trackball endpoints: choose a perpendicular axis.
        {
            delta.w=0;
            if (std::abs(a.x)<0.9f) { delta.x=0; delta.y=a.z; delta.z=-a.y; }
            else { delta.x=-a.z; delta.y=0; delta.z=a.x; }
        }
        delta.normalise(); *this=multiply(delta,*this);
    }
};
}
