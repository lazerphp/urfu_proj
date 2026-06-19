#include "Particle.hpp"
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
    sf::CircleShape shape(m_radius);
    shape.setOrigin({m_radius, m_radius});
    shape.setFillColor(m_color);
    shape.setPosition(m_position);
    window.draw(shape);
}
