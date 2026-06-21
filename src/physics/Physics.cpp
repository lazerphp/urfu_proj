#include "physics/Physics.hpp"
#include <cmath>
#include <algorithm>

namespace Physics
{

bool isPointInPolygon(Vector2f p, const std::vector<Vector2f>& polygon)
{
    int n = polygon.size();
    if (n < 3) return false;
    bool inside = false;
    for (int i = 0, j = n - 1; i < n; j = i++)
    {
        if (((polygon[i].y > p.y) != (polygon[j].y > p.y)) &&
            (p.x < (polygon[j].x - polygon[i].x) * (p.y - polygon[i].y) / (polygon[j].y - polygon[i].y) + polygon[i].x))
        {
            inside = !inside;
        }
    }
    return inside;
}

bool isPointInCorridor(Vector2f p, const std::vector<Config::Compartment>& compartments)
{
    for (const auto& comp : compartments)
    {
        if (isPointInPolygon(p, comp.polygon))
        {
            return true;
        }
    }
    return false;
}

Vector2f closestPointOnSegment(Vector2f p, Vector2f a, Vector2f b)
{
    Vector2f ab = b - a;
    Vector2f ap = p - a;
    float abLenSq = ab.x * ab.x + ab.y * ab.y;
    if (abLenSq < 0.0001f) return a;
    
    float t = (ap.x * ab.x + ap.y * ab.y) / abLenSq;
    t = std::max(0.f, std::min(1.f, t));
    return a + t * ab;
}

void resolveCollisionWithSegment(Particle& p, Vector2f a, Vector2f b, float radius, bool isBoundary, const std::vector<Config::Compartment>& compartments)
{
    Vector2f pos = p.getPosition();
    Vector2f vel = p.getVelocity();
    Vector2f closest = closestPointOnSegment(pos, a, b);
    Vector2f diff = pos - closest;
    float distSq = diff.x * diff.x + diff.y * diff.y;
    float rSq = radius * radius;
    
    if (distSq < rSq && distSq > 0.00001f)
    {
        float dist = std::sqrt(distSq);
        Vector2f normal = diff / dist;
        
        if (isBoundary)
        {
            Vector2f testPos = closest + normal * 0.1f;
            if (!isPointInCorridor(testPos, compartments))
            {
                normal = -normal;
            }
        }

        p.setPosition(closest + normal * radius);
        
        float vn = vel.x * normal.x + vel.y * normal.y;
        if (vn < 0.f)
        {
            p.setVelocity(vel - 2.f * vn * normal);
        }
    }
}

void resolveCollisionWithZone(Particle& p, const std::vector<Vector2f>& polygon, float radius, bool blockEntry)
{
    int n = polygon.size();
    if (n < 3) return;

    for (int i = 0; i < n; ++i)
    {
        Vector2f a = polygon[i];
        Vector2f b = polygon[(i + 1) % n];

        Vector2f pos = p.getPosition();
        Vector2f vel = p.getVelocity();
        Vector2f closest = closestPointOnSegment(pos, a, b);
        Vector2f diff = pos - closest;
        float distSq = diff.x * diff.x + diff.y * diff.y;
        float rSq = radius * radius;

        if (distSq < rSq && distSq > 0.00001f)
        {
            float dist = std::sqrt(distSq);
            Vector2f normal = diff / dist;

            Vector2f testPos = closest + normal * 0.1f;
            bool testInside = isPointInPolygon(testPos, polygon);

            if (blockEntry)
            {
                if (testInside)
                {
                    normal = -normal;
                }
            }
            else // blockExit
            {
                if (!testInside)
                {
                    normal = -normal;
                }
            }

            p.setPosition(closest + normal * radius);

            float vn = vel.x * normal.x + vel.y * normal.y;
            if (vn < 0.f)
            {
                p.setVelocity(vel - 2.f * vn * normal);
            }
        }
    }
}

Vector2f calculateLJForce(const Vector2f& r_vec, float epsilon, float sigma, float cutoff)
{
    float rSq = r_vec.x * r_vec.x + r_vec.y * r_vec.y;
    if (rSq < 0.0001f) return {0.f, 0.f};

    float r = std::sqrt(rSq);
    if (r > cutoff) return {0.f, 0.f};

    // Cap the minimum distance at 0.5 * sigma to prevent division by zero or extreme forces
    float effective_r = std::max(r, 0.5f * sigma);
    float s_r = sigma / effective_r;
    float s_r2 = s_r * s_r;
    float s_r6 = s_r2 * s_r2 * s_r2;
    float s_r12 = s_r6 * s_r6;

    // F_ij = (24 * epsilon / r^2) * (2 * (sigma/r)^12 - (sigma/r)^6) * r_vec
    // When effective_r < 2^(1/6) * sigma, the force factor is positive, yielding a repulsive force (pointing in direction of r_vec).
    float forceFactor = (24.f * epsilon / (effective_r * effective_r)) * (2.f * s_r12 - s_r6);
    return r_vec * forceFactor;
}

Vector2f calculateRadialFieldAcc(const Vector2f& p, const Vector2f& center, float intensity, float minRadius)
{
    Vector2f d_vec = p - center;
    float dSq = d_vec.x * d_vec.x + d_vec.y * d_vec.y;
    if (dSq < 0.0001f) return {0.f, 0.f};

    float d = std::sqrt(dSq);
    float effective_d = std::max(d, minRadius);

    // a = (intensity / d^3) * d_vec
    float factor = intensity / (effective_d * effective_d * effective_d);
    return d_vec * factor;
}

Vector2f calculateSegmentFieldAcc(const Vector2f& p, const Vector2f& a, const Vector2f& b, float intensity)
{
    Vector2f closest = closestPointOnSegment(p, a, b);
    Vector2f d_vec = p - closest;
    float dSq = d_vec.x * d_vec.x + d_vec.y * d_vec.y;
    if (dSq < 0.0001f) return {0.f, 0.f};

    float d = std::sqrt(dSq);
    float effective_d = std::max(d, 5.0f); // 5.0f threshold prevents singularity at segment contact

    // a = (intensity / d^2) * d_vec
    float factor = intensity / (effective_d * effective_d);
    return d_vec * factor;
}

} // namespace Physics
