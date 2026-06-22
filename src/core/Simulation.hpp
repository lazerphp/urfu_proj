#pragma once
#include "core/Config.hpp"
#include "entities/Particle.hpp"
#include "entities/Environment.hpp"
#include "entities/Field.hpp"
#include <vector>
#include <memory>
#include <string>

class TargetZone;

class Simulation
{
public:
    Simulation(const std::string& configPath = "config.yaml");
    void run(); // Pure physics headless loop
    void update(); // Step physical state forward

    const Config& getConfig() const { return m_config; }
    const Environment& getEnvironment() const { return m_environment; }
    const std::vector<Particle>& getParticles() const { return m_particles; }
    const std::vector<std::unique_ptr<PotentialField>>& getFields() const { return m_fields; }

    float getVirtualTime() const { return m_virtualTime; }
    int getReachedCount() const { return m_reachedCount; }
    bool isFinished() const;
    bool isSuccess() const;
    void step();
    void reportStatus();
    void reportResult() const;

private:
    void initParticles();
    void updatePhysics(float dt);

private:
    Config m_config;
    Environment m_environment;
    std::vector<Particle> m_particles;
    std::vector<std::unique_ptr<PotentialField>> m_fields;

    const TargetZone* m_targetZone{nullptr};
    float m_virtualTime{0.f};
    int m_reachedCount{0};
    float m_lastReportTime{-1.f};
    bool m_machineReadableOutput{false};
};
