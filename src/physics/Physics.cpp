#include "physics/Physics.hpp"
#include <cmath>
#include <algorithm>

namespace Physics
{

bool isPointInPolygon(sf::Vector2f p, const std::vector<sf::Vector2f>& polygon)
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

bool isPointInCorridor(sf::Vector2f p, const std::vector<Config::Compartment>& compartments)
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

sf::Vector2f closestPointOnSegment(sf::Vector2f p, sf::Vector2f a, sf::Vector2f b)
{
    sf::Vector2f ab = b - a;
    sf::Vector2f ap = p - a;
    float abLenSq = ab.x * ab.x + ab.y * ab.y;
    if (abLenSq < 0.0001f) return a;
    
    float t = (ap.x * ab.x + ap.y * ab.y) / abLenSq;
    t = std::max(0.f, std::min(1.f, t));
    return a + t * ab;
}

void resolveCollisionWithSegment(Particle& p, sf::Vector2f a, sf::Vector2f b, float radius, bool isBoundary, const std::vector<Config::Compartment>& compartments)
{
    sf::Vector2f pos = p.getPosition();
    sf::Vector2f vel = p.getVelocity();
    sf::Vector2f closest = closestPointOnSegment(pos, a, b);
    sf::Vector2f diff = pos - closest;
    float distSq = diff.x * diff.x + diff.y * diff.y;
    float rSq = radius * radius;
    
    if (distSq < rSq && distSq > 0.00001f)
    {
        float dist = std::sqrt(distSq);
        sf::Vector2f normal = diff / dist;
        
        if (isBoundary)
        {
            sf::Vector2f testPos = closest + normal * 0.1f;
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

} // namespace Physics
