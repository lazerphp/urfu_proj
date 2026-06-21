#include "simulation/Zone.hpp"
#include <SFML/Graphics/ConvexShape.hpp>

Zone::Zone(const std::string& type, const Config::Compartment* compartment)
    : m_type(type)
    , m_compartment(compartment)
{
    if (type == "spawn")
    {
        m_fillColor = sf::Color(59, 130, 246, 40);
        m_outlineColor = sf::Color(59, 130, 246, 150);
    }
    else if (type == "target")
    {
        m_fillColor = sf::Color(16, 185, 129, 40);
        m_outlineColor = sf::Color(16, 185, 129, 150);
    }
    else
    {
        m_fillColor = sf::Color(148, 163, 184, 40);
        m_outlineColor = sf::Color(148, 163, 184, 150);
    }
}

void Zone::update(const std::vector<Particle>& particles)
{
    m_particleCount = 0;
    if (!m_compartment || m_compartment->polygon.empty()) return;

    for (const auto& p : particles)
    {
        if (isPointInPolygon(p.getPosition(), m_compartment->polygon))
        {
            m_particleCount++;
        }
    }
}

void Zone::draw(sf::RenderWindow& window) const
{
    if (!m_compartment || m_compartment->polygon.empty()) return;

    sf::ConvexShape shape(m_compartment->polygon.size());
    for (size_t i = 0; i < m_compartment->polygon.size(); ++i)
    {
        shape.setPoint(i, m_compartment->polygon[i]);
    }
    shape.setFillColor(m_fillColor);
    shape.setOutlineColor(m_outlineColor);
    shape.setOutlineThickness(1.f);
    window.draw(shape);
}

bool Zone::isPointInPolygon(sf::Vector2f p, const std::vector<sf::Vector2f>& polygon) const
{
    int n = polygon.size();
    if (n < 3) return false;
    bool inside = false;
    for (int i = 0, j = n - 1; i < n; j = i++)
    {
        if (((polygon[i].y > p.y) != (polygon[j].y > p.y)) &&
            (p.x < (polygon[j].x - polygon[i].x) * (p.y - polygon[i].y) / (polygon[j].y - polygon[i].y) + polygon[i].x))
        {
            inside = !inside;
        }
    }
    return inside;
}
