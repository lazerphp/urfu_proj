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

} // namespace Physics
