#pragma once
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

class Particle
{
public:
    Particle(sf::Vector2f position, sf::Vector2f velocity, float radius, sf::Color color);

    void update(float dt);
    void draw(sf::RenderWindow& window) const;

    // Getters and setters
    sf::Vector2f getPosition() const { return m_position; }
    void setPosition(sf::Vector2f pos) { m_position = pos; }

    sf::Vector2f getVelocity() const { return m_velocity; }
    void setVelocity(sf::Vector2f vel) { m_velocity = vel; }

    float getRadius() const { return m_radius; }
    sf::Color getColor() const { return m_color; }

private:
    sf::Vector2f m_position;
    sf::Vector2f m_velocity;
    float m_radius;
    sf::Color m_color;
};
