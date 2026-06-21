#pragma once

struct Vector2f
{
    float x{0.f};
    float y{0.f};

    Vector2f() = default;
    Vector2f(float x, float y) : x(x), y(y) {}

    Vector2f operator-() const { return {-x, -y}; }
    Vector2f operator+(const Vector2f& other) const { return {x + other.x, y + other.y}; }
    Vector2f operator-(const Vector2f& other) const { return {x - other.x, y - other.y}; }
    Vector2f operator*(float scalar) const { return {x * scalar, y * scalar}; }
    Vector2f operator/(float scalar) const { return {x / scalar, y / scalar}; }

    Vector2f& operator+=(const Vector2f& other) { x += other.x; y += other.y; return *this; }
    Vector2f& operator-=(const Vector2f& other) { x -= other.x; y -= other.y; return *this; }
    Vector2f& operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }
    Vector2f& operator/=(float scalar) { x /= scalar; y /= scalar; return *this; }
};

inline Vector2f operator*(float scalar, const Vector2f& vec)
{
    return vec * scalar;
}
