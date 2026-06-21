#include "entities/Zone.hpp"
#include "physics/Physics.hpp"

void SpawnZone::processParticles(std::vector<Particle>& particles)
{
    if (!getCompartment()) return;

    for (auto& p : particles)
    {
        if (!p.hasExitedSpawnZone())
        {
            if (!Physics::isPointInPolygon(p.getPosition(), getCompartment()->polygon))
            {
                p.setExitedSpawnZone(true);
            }
        }
    }
}

void TargetZone::processParticles(std::vector<Particle>& particles)
{
    m_particleCount = 0;
    if (!getCompartment()) return;

    for (auto& p : particles)
    {
        bool inside = Physics::isPointInPolygon(p.getPosition(), getCompartment()->polygon);
        if (inside)
        {
            m_particleCount++;
            if (!p.hasEnteredTargetZone())
            {
                p.setEnteredTargetZone(true);
            }
        }
    }
}
