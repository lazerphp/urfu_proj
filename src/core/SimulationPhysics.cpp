#include "core/Simulation.hpp"
#include "physics/Physics.hpp"

#include <cmath>
#include <vector>

void Simulation::updatePhysics(float dt)
{
    size_t numParticles = m_particles.size();
    std::vector<Vector2f> accelerations(numParticles, {0.f, 0.f});

    if (m_config.ljEnabled)
    {
        for (size_t i = 0; i < numParticles; ++i)
        {
            for (size_t j = i + 1; j < numParticles; ++j)
            {
                Vector2f r_vec = m_particles[i].getPosition() - m_particles[j].getPosition();
                Vector2f force = Physics::calculateLJForce(r_vec, m_config.ljEpsilon, m_config.ljSigma, m_config.ljCutoff);
                accelerations[i] += force;
                accelerations[j] -= force;
            }
        }
    }

    for (size_t i = 0; i < numParticles; ++i)
    {
        Vector2f pos = m_particles[i].getPosition();
        for (const auto& field : m_fields)
        {
            accelerations[i] += field->calculateAcceleration(pos);
        }

        float accSq = accelerations[i].x * accelerations[i].x + accelerations[i].y * accelerations[i].y;
        float maxAcc = m_config.ljMaxAcceleration;
        if (accSq > maxAcc * maxAcc)
        {
            float accMag = std::sqrt(accSq);
            accelerations[i] = (accelerations[i] / accMag) * maxAcc;
        }
    }

    for (size_t i = 0; i < numParticles; ++i)
    {
        auto& p = m_particles[i];
        p.setAcceleration(accelerations[i]);
        p.update(dt);

        for (const auto& wall : m_environment.getBoundarySegments())
        {
            Physics::resolveCollisionWithSegment(p, wall.start, wall.end, m_config.particleRadius, true, m_config.compartments);
        }

        for (const auto& obs : m_environment.getObstacles())
        {
            Physics::resolveCollisionWithSegment(p, obs.start, obs.end, m_config.particleRadius, false, m_config.compartments);
        }

        for (const auto& zone : m_environment.getZones())
        {
            zone->resolvePhysics(p, m_config.particleRadius);
        }
    }
}
