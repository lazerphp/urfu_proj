#pragma once
#include <SFML/Graphics.hpp>
#include "core/Config.hpp"
#include "entities/Particle.hpp"
#include "entities/Environment.hpp"
#include <vector>

class TargetZone;

class Simulation
{
public:
    Simulation();
    void run(); // Pure physics headless loop
    void update(); // Step physical state forward

    const Config& getConfig() const { return m_config; }
    const Environment& getEnvironment() const { return m_environment; }
    const std::vector<Particle>& getParticles() const { return m_particles; }

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

    const TargetZone* m_targetZone{nullptr};
    float m_virtualTime{0.f};
    int m_reachedCount{0};
    float m_lastReportTime{-1.f};
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

    // HUD Text
    sf::Font m_font;
    bool m_fontLoaded{false};
};
