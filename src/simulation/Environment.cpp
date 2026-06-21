#include "simulation/Environment.hpp"
#include <SFML/Graphics/VertexArray.hpp>
#include <iostream>

Environment::Environment(const Config& config)
    : m_obstacles(config.obstacles)
{
    buildBoundarySegments(config);
}

void Environment::update(const std::vector<Particle>& particles)
{
    m_particlesInTarget = 0;
    for (auto& zone : m_zones)
    {
        zone.update(particles);
        if (zone.getType() == "target")
        {
            m_particlesInTarget = zone.getParticleCount();
        }
    }
}

void Environment::draw(sf::RenderWindow& window) const
{
    for (const auto& zone : m_zones)
    {
        zone.draw(window);
    }

    if (!m_obstacles.empty())
    {
        sf::VertexArray wallLines(sf::PrimitiveType::Lines);
        for (const auto& wall : m_obstacles)
        {
            wallLines.append(sf::Vertex(wall.start, sf::Color::White));
            wallLines.append(sf::Vertex(wall.end, sf::Color::White));
        }
        window.draw(wallLines);
    }

    if (!m_boundarySegments.empty())
    {
        sf::VertexArray polyLines(sf::PrimitiveType::Lines);
        for (const auto& seg : m_boundarySegments)
        {
            polyLines.append(sf::Vertex(seg.start, sf::Color::White));
            polyLines.append(sf::Vertex(seg.end, sf::Color::White));
        }
        window.draw(polyLines);
    }
}

void Environment::buildBoundarySegments(const Config& config)
{
    m_boundarySegments.clear();
    m_zones.clear();
    m_spawnZonePtr = nullptr;

    for (const auto& zoneConf : config.zones)
    {
        const Config::Compartment* matchedComp = nullptr;
        for (const auto& comp : config.compartments)
        {
            if (comp.name == zoneConf.compartment)
            {
                matchedComp = &comp;
                break;
            }
        }
        if (matchedComp)
        {
            m_zones.emplace_back(zoneConf.type, matchedComp);
        }
    }

    for (auto& zone : m_zones)
    {
        if (zone.getType() == "spawn")
        {
            m_spawnZonePtr = &zone;
        }
    }

    struct SegmentInfo {
        sf::Vector2f start;
        sf::Vector2f end;
        bool isShared = false;
    };
    std::vector<SegmentInfo> allSegments;

    for (const auto& comp : config.compartments)
    {
        size_t n = comp.polygon.size();
        for (size_t i = 0; i < n; ++i)
        {
            allSegments.push_back({
                comp.polygon[i],
                comp.polygon[(i + 1) % n],
                false
            });
        }
    }

    auto approxEqual = [](sf::Vector2f p1, sf::Vector2f p2) {
        float dx = p1.x - p2.x;
        float dy = p1.y - p2.y;
        return (dx * dx + dy * dy) < 0.1f;
    };

    for (size_t i = 0; i < allSegments.size(); ++i)
    {
        for (size_t j = i + 1; j < allSegments.size(); ++j)
        {
            bool matchNormal = approxEqual(allSegments[i].start, allSegments[j].start) && 
                              approxEqual(allSegments[i].end, allSegments[j].end);
            bool matchReverse = approxEqual(allSegments[i].start, allSegments[j].end) && 
                               approxEqual(allSegments[i].end, allSegments[j].start);
            if (matchNormal || matchReverse)
            {
                allSegments[i].isShared = true;
                allSegments[j].isShared = true;
            }
        }
    }

    for (const auto& seg : allSegments)
    {
        if (!seg.isShared)
        {
            m_boundarySegments.push_back({seg.start, seg.end});
        }
    }
}
