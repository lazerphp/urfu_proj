#include "simulation/Particle.hpp"
#include <SFML/Graphics/CircleShape.hpp>

Particle::Particle(sf::Vector2f position, sf::Vector2f velocity, float radius, sf::Color color)
    : m_position(position)
    , m_velocity(velocity)
    , m_radius(radius)
    , m_color(color)
{
}

void Particle::update(float dt)
{
    m_position += m_velocity * dt;
}

void Particle::draw(sf::RenderWindow& window) const
{
    sf::CircleShape circle(m_radius);
    circle.setPosition(m_position);
    circle.setOrigin({m_radius, m_radius});
    circle.setFillColor(m_color);
    window.draw(circle);
}
