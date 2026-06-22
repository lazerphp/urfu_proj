#include "core/Simulation.hpp"
#include "physics/Physics.hpp"

#include <iostream>
#include <random>

void Simulation::initParticles()
{
    const Zone* spawnZone = nullptr;
    for (const auto& zone : m_environment.getZones())
    {
        if (zone->getZoneType() == Config::ZoneType::Spawn)
        {
            spawnZone = zone.get();
            break;
        }
    }

    if (!spawnZone || !spawnZone->getCompartment() || spawnZone->getCompartment()->polygon.empty())
    {
        std::cerr << "Warning: Spawn zone is empty or not found. No particles spawned." << std::endl;
        return;
    }

    const auto* comp = spawnZone->getCompartment();
    float minX = comp->polygon[0].x;
    float maxX = comp->polygon[0].x;
    float minY = comp->polygon[0].y;
    float maxY = comp->polygon[0].y;

    for (const auto& vertex : comp->polygon)
    {
        minX = std::min(minX, vertex.x);
        maxX = std::max(maxX, vertex.x);
        minY = std::min(minY, vertex.y);
        maxY = std::max(maxY, vertex.y);
    }

    float padding = m_config.particleRadius + 5.f;
    if (maxX - minX > 2.f * padding)
    {
        minX += padding;
        maxX -= padding;
    }
    if (maxY - minY > 2.f * padding)
    {
        minY += padding;
        maxY -= padding;
    }

    std::mt19937 gen(m_config.randomSeed);
    std::uniform_real_distribution<float> distX(minX, maxX);
    std::uniform_real_distribution<float> distY(minY, maxY);
    std::uniform_real_distribution<float> distSpeed(120.f, 220.f);
    std::uniform_real_distribution<float> distAngle(0.f, 2.f * 3.14159265f);

    m_particles.clear();
    m_particles.reserve(m_config.particleCount);

    int attempts = 0;
    int maxAttempts = m_config.particleCount * 200;

    while (m_particles.size() < static_cast<size_t>(m_config.particleCount) && attempts < maxAttempts)
    {
        attempts++;

        Vector2f pos(distX(gen), distY(gen));

        if (Physics::isPointInPolygon(pos, comp->polygon))
        {
            bool insideObstacle = false;
            for (const auto& obs : m_environment.getObstacles())
            {
                Vector2f closest = Physics::closestPointOnSegment(pos, obs.start, obs.end);
                Vector2f diff = pos - closest;
                float distSq = diff.x * diff.x + diff.y * diff.y;
                if (distSq < m_config.particleRadius * m_config.particleRadius)
                {
                    insideObstacle = true;
                    break;
                }
            }

            if (!insideObstacle)
            {
                float speed = distSpeed(gen);
                float angle = distAngle(gen);
                Vector2f vel(speed * std::cos(angle), speed * std::sin(angle));

                m_particles.emplace_back(pos, vel, m_config.particleRadius);
            }
        }
    }
}
