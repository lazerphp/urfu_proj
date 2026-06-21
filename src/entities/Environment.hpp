#pragma once
#include "core/Types.hpp"
#include <vector>
#include "core/Config.hpp"
#include "entities/Zone.hpp"
#include <memory>

struct WallSegment
{
    Vector2f start;
    Vector2f end;
};

class Environment
{
public:
    Environment(const Config& config);

    const std::vector<WallSegment>& getBoundarySegments() const { return m_boundarySegments; }
    const std::vector<Config::ObstacleSegment>& getObstacles() const { return m_obstacles; }
    const Zone* getSpawnZone() const { return m_spawnZonePtr; }
    
    std::vector<std::unique_ptr<Zone>>& getZones() { return m_zones; }
    const std::vector<std::unique_ptr<Zone>>& getZones() const { return m_zones; }

    int getParticlesInTarget() const;

private:
    void buildBoundarySegments(const Config& config);

private:
    std::vector<WallSegment> m_boundarySegments;
    std::vector<Config::ObstacleSegment> m_obstacles;
    std::vector<std::unique_ptr<Zone>> m_zones;
    const Zone* m_spawnZonePtr{nullptr};
};
