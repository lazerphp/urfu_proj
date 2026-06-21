#pragma once
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Color.hpp>
#include "core/Config.hpp"
#include "simulation/Particle.hpp"
#include <string>
#include <vector>

class Zone
{
public:
    Zone(const std::string& type, const Config::Compartment* compartment);

    void update(const std::vector<Particle>& particles);
    void draw(sf::RenderWindow& window) const;

    const std::string& getType() const { return m_type; }
    const Config::Compartment* getCompartment() const { return m_compartment; }
    int getParticleCount() const { return m_particleCount; }

private:
    bool isPointInPolygon(sf::Vector2f p, const std::vector<sf::Vector2f>& polygon) const;

private:
    std::string m_type;
    const Config::Compartment* m_compartment{nullptr};
    sf::Color m_fillColor;
    sf::Color m_outlineColor;
    int m_particleCount{0};
};
