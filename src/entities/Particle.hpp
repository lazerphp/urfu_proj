#pragma once
#include "core/Types.hpp"

class Particle
{
public:
    Particle(Vector2f position, Vector2f velocity, float radius);

    void update(float dt);

    // Getters and setters
    Vector2f getPosition() const { return m_position; }
    void setPosition(Vector2f pos) { m_position = pos; }

    Vector2f getVelocity() const { return m_velocity; }
    void setVelocity(Vector2f vel) { m_velocity = vel; }

    float getRadius() const { return m_radius; }

    bool hasExitedSpawnZone() const { return m_hasExitedSpawn; }
    void setExitedSpawnZone(bool val) { m_hasExitedSpawn = val; }

    bool hasEnteredTargetZone() const { return m_hasEnteredTarget; }
    void setEnteredTargetZone(bool val) { m_hasEnteredTarget = val; }

private:
    Vector2f m_position;
    Vector2f m_velocity;
    float m_radius;
    bool m_hasExitedSpawn{false};
    bool m_hasEnteredTarget{false};
};
