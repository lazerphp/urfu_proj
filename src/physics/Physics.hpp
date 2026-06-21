#pragma once
#include <SFML/System/Vector2.hpp>
#include <vector>
#include "simulation/Particle.hpp"
#include "core/Config.hpp"

namespace Physics
{
    bool isPointInPolygon(sf::Vector2f p, const std::vector<sf::Vector2f>& polygon);
    
    bool isPointInCorridor(sf::Vector2f p, const std::vector<Config::Compartment>& compartments);
    
    sf::Vector2f closestPointOnSegment(sf::Vector2f p, sf::Vector2f a, sf::Vector2f b);
    
    void resolveCollisionWithSegment(Particle& p, sf::Vector2f a, sf::Vector2f b, float radius, bool isBoundary, const std::vector<Config::Compartment>& compartments);
}
