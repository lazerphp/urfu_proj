#include "SpawnZone.hpp"
#include <SFML/Graphics/RectangleShape.hpp>

SpawnZone::SpawnZone(float x, float y, float width, float height, bool enabled)
    : m_x(x)
    , m_y(y)
    , m_width(width)
    , m_height(height)
    , m_enabled(enabled)  // Зачем
{
}

void SpawnZone::draw(sf::RenderWindow& window) const
{
    if (!m_enabled) return;

    sf::RectangleShape spawnRect({m_width, m_height});
    spawnRect.setPosition({m_x, m_y});
    spawnRect.setFillColor(sf::Color(59, 130, 246, 40)); // Blue-500 with alpha 40
    spawnRect.setOutlineColor(sf::Color(59, 130, 246, 150));
    spawnRect.setOutlineThickness(1.f);
    window.draw(spawnRect);
}

sf::Vector2f SpawnZone::getRandomPosition(std::mt19937& gen) const
{
    std::uniform_real_distribution<float> distX(m_x, m_x + m_width);
    std::uniform_real_distribution<float> distY(m_y, m_y + m_height);
    return {distX(gen), distY(gen)};
}
