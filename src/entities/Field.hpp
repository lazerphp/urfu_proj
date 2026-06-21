#pragma once
#include "core/Types.hpp"
#include <SFML/Graphics.hpp>

class PotentialField
{
public:
    virtual ~PotentialField() = default;
    virtual Vector2f calculateAcceleration(const Vector2f& pos) const = 0;
    virtual void render(sf::RenderWindow& window) const = 0;
};

class RadialField : public PotentialField
{
public:
    RadialField(Vector2f center, float intensity, float minRadius);
    Vector2f calculateAcceleration(const Vector2f& pos) const override;
    void render(sf::RenderWindow& window) const override;

private:
    Vector2f m_center;
    float m_intensity;
    float m_minRadius;
};

class SegmentField : public PotentialField
{
public:
    SegmentField(Vector2f start, Vector2f end, float intensity);
    Vector2f calculateAcceleration(const Vector2f& pos) const override;
    void render(sf::RenderWindow& window) const override;

private:
    Vector2f m_start;
    Vector2f m_end;
    float m_intensity;
};
