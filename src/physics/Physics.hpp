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

    // Calculate Lennard-Jones 6-12 force acting on particle i from particle j:
    // F_ij = (24 * epsilon / r^2) * ((sigma/r)^6) * (2 * (sigma/r)^6 - 1) * r_vec
    // where r_vec = pos_i - pos_j
    Vector2f calculateLJForce(const Vector2f& r_vec, float epsilon, float sigma, float cutoff);
}
