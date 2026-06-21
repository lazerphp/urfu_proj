#include "entities/Particle.hpp"

Particle::Particle(Vector2f position, Vector2f velocity, float radius)
    : m_position(position)
    , m_velocity(velocity)
    , m_radius(radius)
    , m_acceleration(0.f, 0.f)
{
}

void Particle::update(float dt)
{
    m_velocity += m_acceleration * dt;
    m_position += m_velocity * dt;
}
