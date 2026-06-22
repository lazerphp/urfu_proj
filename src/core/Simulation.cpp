#include "core/Simulation.hpp"

#include <cmath>
#include <iostream>

Simulation::Simulation(const std::string& configPath)
    : m_config(Config::loadFromFile(configPath))
    , m_environment(m_config)
{
    for (const auto& field : m_config.radialFields)
    {
        m_fields.push_back(std::make_unique<RadialField>(field.center, field.intensity, field.minRadius));
    }
    for (const auto& field : m_config.segmentFields)
    {
        m_fields.push_back(std::make_unique<SegmentField>(field.start, field.end, field.intensity));
    }

    initParticles();
    m_targetZone = m_environment.getTargetZone();
    m_reachedCount = m_targetZone ? m_targetZone->getParticleCount() : 0;
}

void Simulation::run()
{
    m_machineReadableOutput = true;

    while (!isFinished())
    {
        step();
        reportStatus();
    }

    reportResult();
}

bool Simulation::isFinished() const
{
    const float maxVirtualTime = 1000.f;
    return m_reachedCount >= m_config.particleCount || m_virtualTime >= maxVirtualTime;
}

bool Simulation::isSuccess() const
{
    return m_reachedCount >= m_config.particleCount;
}

void Simulation::step()
{
    update();
    m_virtualTime += m_config.dt * m_config.physicsStepsPerFrame;
    m_reachedCount = m_targetZone ? m_targetZone->getParticleCount() : 0;
}

void Simulation::reportStatus()
{
    if (m_virtualTime - m_lastReportTime >= 1.0f)
    {
        if (m_machineReadableOutput)
        {
            std::cout << "SIM_STATUS"
                      << " virtual_time=" << m_virtualTime
                      << " reached=" << m_reachedCount
                      << " total=" << m_config.particleCount
                      << std::endl;
        }
        else
        {
            std::cout << "[Virtual Time: " << std::round(m_virtualTime) << "s] Reached target: "
                      << m_reachedCount << "/" << m_config.particleCount << std::endl;
        }
        m_lastReportTime = m_virtualTime;
    }
}

void Simulation::reportResult() const
{
    if (m_machineReadableOutput)
    {
        std::cout << "SIM_RESULT"
                  << " success=" << (isSuccess() ? 1 : 0)
                  << " virtual_time=" << m_virtualTime
                  << " reached=" << m_reachedCount
                  << " total=" << m_config.particleCount
                  << std::endl;
        return;
    }

    if (isSuccess())
    {
        std::cout << "Success: All " << m_config.particleCount << " particles reached the destination reservoir!" << std::endl;
        std::cout << "Total Virtual Time: " << m_virtualTime << " seconds." << std::endl;
    }
    else
    {
        std::cout << "Timeout: Simulation ended after " << m_virtualTime
                  << " virtual seconds. Particles reached: " << m_reachedCount << "/" << m_config.particleCount << std::endl;
    }
}

void Simulation::update()
{
    for (int i = 0; i < m_config.physicsStepsPerFrame; ++i)
    {
        updatePhysics(m_config.dt);
    }

    for (auto& zone : m_environment.getZones())
    {
        zone->processParticles(m_particles);
    }
}
