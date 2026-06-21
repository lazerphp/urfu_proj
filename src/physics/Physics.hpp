#pragma once
#include "core/Types.hpp"
#include <vector>
#include "entities/Particle.hpp"
#include "core/Config.hpp"

namespace Physics
{
    bool isPointInPolygon(Vector2f p, const std::vector<Vector2f>& polygon);
    
    bool isPointInCorridor(Vector2f p, const std::vector<Config::Compartment>& compartments);
    
    Vector2f closestPointOnSegment(Vector2f p, Vector2f a, Vector2f b);
    
    void resolveCollisionWithSegment(Particle& p, Vector2f a, Vector2f b, float radius, bool isBoundary, const std::vector<Config::Compartment>& compartments);
    
    void resolveCollisionWithZone(Particle& p, const std::vector<Vector2f>& polygon, float radius, bool blockEntry);
}
