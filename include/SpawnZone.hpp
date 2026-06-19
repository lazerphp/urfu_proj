#pragma once
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
#include <random>

class SpawnZone
{
public:
    SpawnZone() = default;
    SpawnZone(float x, float y, float width, float height, bool enabled = true);

    void draw(sf::RenderWindow& window) const;
    sf::Vector2f getRandomPosition(std::mt19937& gen) const;

    bool isEnabled() const { return m_enabled; }
    float getX() const { return m_x; }
    float getY() const { return m_y; }
    float getWidth() const { return m_width; }
    float getHeight() const { return m_height; }

private:
    float m_x{0.f};
    float m_y{0.f};
    float m_width{0.f};
    float m_height{0.f};
    bool m_enabled{false};
};
