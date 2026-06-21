#pragma once
#include <SFML/Graphics.hpp>
#include "core/Config.hpp"
#include "entities/Particle.hpp"
#include "entities/Environment.hpp"
#include <vector>

class Simulation
{
public:
    Simulation();
    void run(); // Pure physics headless loop
    void update(); // Step physical state forward

    const Config& getConfig() const { return m_config; }
    const Environment& getEnvironment() const { return m_environment; }
    const std::vector<Particle>& getParticles() const { return m_particles; }

private:
    void initParticles();
    void updatePhysics(float dt);

private:
    Config m_config;
    Environment m_environment;
    std::vector<Particle> m_particles;
};

class Visualizer
{
public:
    Visualizer(Simulation& sim);
    void run(); // Graphic loop with window and events

private:
    void processEvents();
    void update();
    void render();

    sf::Vector2f snapToGrid(sf::Vector2f position) const;
    void drawGrid();
    void updateGridCursor(sf::Vector2i mousePosition);

private:
    Simulation& m_sim;
    sf::RenderWindow m_window;

    // View manipulation state
    bool m_isDragging{false};
    sf::Vector2i m_lastMousePosition{};

    // Grid cursor preview
    sf::RectangleShape m_gridCursorPreview;
};
