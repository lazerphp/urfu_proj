#pragma once

#include <SFML/Graphics.hpp>
#include "Config.hpp"
#include "Particle.hpp"
#include "Zone.hpp"
#include <optional>
#include <vector>

class Game
{
public:
    Game();
    void run();

private:
    void processEvents();
    void update();
    void updatePhysics(float dt);
    void render();

private:
    Config m_config;
    sf::RenderWindow m_window;

    // View manipulation state
    bool m_isDragging{false};
    sf::Vector2i m_lastMousePosition{};

    // Grid settings
    sf::Vector2f snapToGrid(sf::Vector2f position) const;
    void drawGrid();

    // Grid cursor preview
    sf::RectangleShape m_gridCursorPreview;
    void updateGridCursor(sf::Vector2i mousePosition);

    // Physical simulation elements
    struct WallSegment
    {
        sf::Vector2f start;
        sf::Vector2f end;
    };
    std::vector<WallSegment> m_boundarySegments;
    std::vector<Zone> m_zones;
    const Zone* m_spawnZonePtr{nullptr};

    std::vector<Particle> m_particles;
    void initParticles();
    void buildBoundarySegments();

    // Physics helper functions
    bool isPointInPolygon(sf::Vector2f p, const std::vector<sf::Vector2f>& polygon) const;
    bool isPointInCorridor(sf::Vector2f p) const;
    sf::Vector2f closestPointOnSegment(sf::Vector2f p, sf::Vector2f a, sf::Vector2f b) const;
    void resolveCollisionWithSegment(Particle& p, sf::Vector2f a, sf::Vector2f b, float radius, bool isBoundary) const;
};
