#pragma once
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
#include <vector>
#include "core/Config.hpp"
#include "simulation/Zone.hpp"
#include "simulation/Particle.hpp"

struct WallSegment
{
    sf::Vector2f start;
    sf::Vector2f end;
};

class Environment
{
public:
    Environment(const Config& config);

    void update(const std::vector<Particle>& particles);
    void draw(sf::RenderWindow& window) const;

    const std::vector<WallSegment>& getBoundarySegments() const { return m_boundarySegments; }
    const std::vector<Config::ObstacleSegment>& getObstacles() const { return m_obstacles; }
    const Zone* getSpawnZone() const { return m_spawnZonePtr; }
    int getParticlesInTarget() const { return m_particlesInTarget; }

private:
    void buildBoundarySegments(const Config& config);

private:
    std::vector<WallSegment> m_boundarySegments;
    std::vector<Config::ObstacleSegment> m_obstacles;
    std::vector<Zone> m_zones;
    const Zone* m_spawnZonePtr{nullptr};
    int m_particlesInTarget{0};
};
