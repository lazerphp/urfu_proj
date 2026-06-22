#pragma once

#include <SFML/Graphics.hpp>
#include "core/Simulation.hpp"

class Visualizer
{
public:
    explicit Visualizer(Simulation& sim);
    void run();

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

    bool m_isDragging{false};
    sf::Vector2i m_lastMousePosition{};

    sf::RectangleShape m_gridCursorPreview;

    sf::Font m_font;
    bool m_fontLoaded{false};
};
