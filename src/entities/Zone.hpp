#pragma once
#include "core/Types.hpp"
#include "core/Config.hpp"
#include "entities/Particle.hpp"
#include <string>
#include <vector>

class Zone
{
public:
    Zone(const std::string& type, const Config::Compartment* compartment)
        : m_type(type), m_compartment(compartment) {}
    virtual ~Zone() = default;

    const std::string& getType() const { return m_type; }
    const Config::Compartment* getCompartment() const { return m_compartment; }

    virtual void processParticles(std::vector<Particle>& particles) {}
    virtual void resolvePhysics(Particle& p, float radius) {}

private:
    std::string m_type;
    const Config::Compartment* m_compartment{nullptr};
};

class SpawnZone : public Zone
{
public:
    SpawnZone(const Config::Compartment* compartment)
        : Zone("spawn", compartment) {}

    void processParticles(std::vector<Particle>& particles) override;
    void resolvePhysics(Particle& p, float radius) override;
};

class TargetZone : public Zone
{
public:
    TargetZone(const Config::Compartment* compartment)
        : Zone("target", compartment) {}

    void processParticles(std::vector<Particle>& particles) override;
    void resolvePhysics(Particle& p, float radius) override;
    int getParticleCount() const { return m_particleCount; }

private:
    int m_particleCount{0};
};
