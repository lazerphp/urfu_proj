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

void SpawnZone::resolvePhysics(Particle& p, float radius)
{
    if (p.hasExitedSpawnZone() && getCompartment())
    {
        Physics::resolveCollisionWithZone(p, getCompartment()->polygon, radius, true);
    }
}

void TargetZone::processParticles(std::vector<Particle>& particles)
{
    m_particleCount = 0;
    if (!getCompartment()) return;

    for (auto& p : particles)
    {
        if (Physics::isPointInPolygon(p.getPosition(), getCompartment()->polygon))
        {
            m_particleCount++;
            if (!p.hasEnteredTargetZone())
            {
                p.setEnteredTargetZone(true);
            }
        }
    }
}

void TargetZone::resolvePhysics(Particle& p, float radius)
{
    if (p.hasEnteredTargetZone() && getCompartment())
    {
        Physics::resolveCollisionWithZone(p, getCompartment()->polygon, radius, false);
    }
}
