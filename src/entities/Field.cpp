#include "entities/Field.hpp"
#include "physics/Physics.hpp"
#include <cmath>
#include <algorithm>

namespace {
    sf::Vector2f toSF(const Vector2f& v) {
        return {v.x, v.y};
    }

    sf::Color getColorAt(float x_val, sf::Color baseColor) {
        if (x_val <= 0.25f) {
            float t = x_val / 0.25f;
            return sf::Color(
                static_cast<unsigned char>(255.f * (1.f - t) + baseColor.r * t),
                static_cast<unsigned char>(255.f * (1.f - t) + baseColor.g * t),
                static_cast<unsigned char>(255.f * (1.f - t) + baseColor.b * t)
            );
        }
        return baseColor;
    }
}

// ================= RadialField =================
RadialField::RadialField(Vector2f center, float intensity, float minRadius)
    : m_center(center)
    , m_intensity(intensity)
    , m_minRadius(minRadius)
{
}

Vector2f RadialField::calculateAcceleration(const Vector2f& pos) const
{
    Vector2f d_vec = pos - m_center;
    float dSq = d_vec.x * d_vec.x + d_vec.y * d_vec.y;
    if (dSq < 0.0001f) return {0.f, 0.f};

    float d = std::sqrt(dSq);
    float effective_d = std::max(d, m_minRadius);

    // a = (intensity / d^3) * d_vec
    float factor = m_intensity / (effective_d * effective_d * effective_d);
    return d_vec * factor;
}

void RadialField::render(sf::RenderWindow& window) const
{
    // Ambient reach: larger spread but very pale/low opacity (maxAlpha 110.f) for subtle decay
    float maxRadius = std::min(250.f, std::sqrt(std::abs(m_intensity)) * 3.5f);
    if (maxRadius < 10.f) maxRadius = 70.f;

    // Custom colors: attractive amber/yellow or repulsive sky-blue
    sf::Color baseColor = (m_intensity < 0.f) ? sf::Color(255, 180, 70) : sf::Color(80, 190, 255);

    // Physically-matching inverse-square decay 1/d^2 with gamma correction for human eye perception:
    // f(d) = maxAlpha * sqrt( (1/d^2 - 1/d_max^2) / (1/d_min^2 - 1/d_max^2) )
    const int numBands = 8;
    float x[numBands] = { 0.0f, 0.15f, 0.3f, 0.45f, 0.6f, 0.75f, 0.9f, 1.0f };
    float alphaVal[numBands];
    float maxAlpha = 110.f; // lowered back slightly to keep it soft with gamma correction

    float d_min = m_minRadius;
    float d_max = maxRadius;
    if (d_min >= d_max - 1.f) d_min = d_max * 0.2f;
    float inv_min_sq = 1.f / (d_min * d_min);
    float inv_max_sq = 1.f / (d_max * d_max);
    float denom = inv_min_sq - inv_max_sq;

    for (int i = 0; i < numBands; ++i)
    {
        float d = x[i] * d_max;
        if (d <= d_min)
        {
            alphaVal[i] = maxAlpha;
        }
        else
        {
            float inv_d_sq = 1.f / (d * d);
            float factor = (inv_d_sq - inv_max_sq) / denom;
            alphaVal[i] = maxAlpha * std::sqrt(std::max(0.f, std::min(1.f, factor)));
        }
    }

    const int numSegments = 32;
    sf::Vector2f center = toSF(m_center);

    // 1. Center disc (from radius 0 to x[1]*maxRadius)
    {
        sf::Color c0 = getColorAt(x[0], baseColor); c0.a = static_cast<unsigned char>(alphaVal[0]);
        sf::Color c1 = getColorAt(x[1], baseColor); c1.a = static_cast<unsigned char>(alphaVal[1]);
        float r1 = maxRadius * x[1];

        sf::VertexArray fan(sf::PrimitiveType::TriangleFan, numSegments + 2);
        fan[0] = sf::Vertex(center, c0);
        for (int i = 0; i <= numSegments; ++i)
        {
            float angle = (i * 2.f * 3.14159265f) / numSegments;
            sf::Vector2f pos(
                center.x + r1 * std::cos(angle),
                center.y + r1 * std::sin(angle)
            );
            fan[i + 1] = sf::Vertex(pos, c1);
        }
        window.draw(fan);
    }

    // 2. Concentric ring bands (from band j to j+1)
    for (int j = 1; j < numBands - 1; ++j)
    {
        sf::Color c_j = getColorAt(x[j], baseColor); c_j.a = static_cast<unsigned char>(alphaVal[j]);
        sf::Color c_next = getColorAt(x[j+1], baseColor); c_next.a = static_cast<unsigned char>(alphaVal[j+1]);
        float r_j = maxRadius * x[j];
        float r_next = maxRadius * x[j+1];

        sf::VertexArray strip(sf::PrimitiveType::TriangleStrip, (numSegments + 1) * 2);
        for (int i = 0; i <= numSegments; ++i)
        {
            float angle = (i * 2.f * 3.14159265f) / numSegments;
            float cosA = std::cos(angle);
            float sinA = std::sin(angle);

            sf::Vector2f pos_in(center.x + r_j * cosA, center.y + r_j * sinA);
            sf::Vector2f pos_out(center.x + r_next * cosA, center.y + r_next * sinA);

            strip[2 * i] = sf::Vertex(pos_in, c_j);
            strip[2 * i + 1] = sf::Vertex(pos_out, c_next);
        }
        window.draw(strip);
    }
}

// ================= SegmentField =================
SegmentField::SegmentField(Vector2f start, Vector2f end, float intensity)
    : m_start(start)
    , m_end(end)
    , m_intensity(intensity)
{
}

Vector2f SegmentField::calculateAcceleration(const Vector2f& pos) const
{
    Vector2f closest = Physics::closestPointOnSegment(pos, m_start, m_end);
    Vector2f d_vec = pos - closest;
    float dSq = d_vec.x * d_vec.x + d_vec.y * d_vec.y;
    if (dSq < 0.0001f) return {0.f, 0.f};

    float d = std::sqrt(dSq);
    float effective_d = std::max(d, 5.0f); // 5.0f threshold prevents singularity at segment contact

    // a = (intensity / d^2) * d_vec
    float factor = m_intensity / (effective_d * effective_d);
    return d_vec * factor;
}

void SegmentField::render(sf::RenderWindow& window) const
{
    sf::Vector2f a = toSF(m_start);
    sf::Vector2f b = toSF(m_end);
    sf::Vector2f dir = b - a;
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len < 0.001f) return;

    sf::Vector2f tangent = dir / len;
    sf::Vector2f normal(-tangent.y, tangent.x);

    // Ambient reach: larger spread but very pale/low opacity (maxAlpha 90.f) for subtle decay
    float maxWidth = std::min(150.f, std::sqrt(std::abs(m_intensity)) * 2.5f);
    if (maxWidth < 5.f) maxWidth = 50.f;

    // Custom colors: attractive amber/yellow or repulsive sky-blue
    sf::Color baseColor = (m_intensity < 0.f) ? sf::Color(255, 180, 70) : sf::Color(80, 190, 255);

    // Physically-matching inverse-linear decay 1/d with gamma correction for human eye perception:
    // f(d) = maxAlpha * sqrt( (1/d - 1/d_max) / (1/d_min - 1/d_max) )
    const int numBands = 8;
    float x[numBands] = { 0.0f, 0.15f, 0.3f, 0.45f, 0.6f, 0.75f, 0.9f, 1.0f };
    float alphaVal[numBands];
    float maxAlpha = 90.f; // lowered back slightly to keep it soft with gamma correction

    float d_min = 5.0f; // matches singularity limit in physics
    float d_max = maxWidth;
    if (d_min >= d_max - 1.f) d_min = d_max * 0.2f;
    float inv_min = 1.f / d_min;
    float inv_max = 1.f / d_max;
    float denom = inv_min - inv_max;

    for (int i = 0; i < numBands; ++i)
    {
        float d = x[i] * d_max;
        if (d <= d_min)
        {
            alphaVal[i] = maxAlpha;
        }
        else
        {
            float inv_d = 1.f / d;
            float factor = (inv_d - inv_max) / denom;
            alphaVal[i] = maxAlpha * std::sqrt(std::max(0.f, std::min(1.f, factor)));
        }
    }

    const int numCapSegments = 24;
    float baseAngle = std::atan2(tangent.y, tangent.x);

    for (int j = 0; j < numBands - 1; ++j)
    {
        sf::Color c_in = getColorAt(x[j], baseColor); c_in.a = static_cast<unsigned char>(alphaVal[j]);
        sf::Color c_out = getColorAt(x[j+1], baseColor); c_out.a = static_cast<unsigned char>(alphaVal[j+1]);
        float w_in = maxWidth * x[j];
        float w_out = maxWidth * x[j+1];

        // A. Left-side body strip for this band (zero overlap)
        sf::VertexArray leftStrip(sf::PrimitiveType::TriangleStrip, 4);
        leftStrip[0] = sf::Vertex(a + normal * w_in, c_in);
        leftStrip[1] = sf::Vertex(a + normal * w_out, c_out);
        leftStrip[2] = sf::Vertex(b + normal * w_in, c_in);
        leftStrip[3] = sf::Vertex(b + normal * w_out, c_out);
        window.draw(leftStrip);

        // B. Right-side body strip for this band (zero overlap)
        sf::VertexArray rightStrip(sf::PrimitiveType::TriangleStrip, 4);
        rightStrip[0] = sf::Vertex(a - normal * w_in, c_in);
        rightStrip[1] = sf::Vertex(a - normal * w_out, c_out);
        rightStrip[2] = sf::Vertex(b - normal * w_in, c_in);
        rightStrip[3] = sf::Vertex(b - normal * w_out, c_out);
        window.draw(rightStrip);

        // C. Cap A semi-circular ring band for this band (zero overlap)
        sf::VertexArray capA(sf::PrimitiveType::TriangleStrip, (numCapSegments + 1) * 2);
        for (int i = 0; i <= numCapSegments; ++i)
        {
            float angle = baseAngle + 3.14159265f * 0.5f + (i * 3.14159265f) / numCapSegments;
            float cosA = std::cos(angle);
            float sinA = std::sin(angle);

            sf::Vector2f pos_in(a.x + w_in * cosA, a.y + w_in * sinA);
            sf::Vector2f pos_out(a.x + w_out * cosA, a.y + w_out * sinA);

            capA[2 * i] = sf::Vertex(pos_in, c_in);
            capA[2 * i + 1] = sf::Vertex(pos_out, c_out);
        }
        window.draw(capA);

        // D. Cap B semi-circular ring band for this band (zero overlap)
        sf::VertexArray capB(sf::PrimitiveType::TriangleStrip, (numCapSegments + 1) * 2);
        for (int i = 0; i <= numCapSegments; ++i)
        {
            float angle = baseAngle - 3.14159265f * 0.5f + (i * 3.14159265f) / numCapSegments;
            float cosA = std::cos(angle);
            float sinA = std::sin(angle);

            sf::Vector2f pos_in(b.x + w_in * cosA, b.y + w_in * sinA);
            sf::Vector2f pos_out(b.x + w_out * cosA, b.y + w_out * sinA);

            capB[2 * i] = sf::Vertex(pos_in, c_in);
            capB[2 * i + 1] = sf::Vertex(pos_out, c_out);
        }
        window.draw(capB);
    }

    // Draw the center plate line (white and thin like a filament of a bulb)
    sf::VertexArray centerLine(sf::PrimitiveType::Lines, 2);
    sf::Color lineCol = sf::Color::White;
    lineCol.a = 230; 
    centerLine[0] = sf::Vertex(a, lineCol);
    centerLine[1] = sf::Vertex(b, lineCol);
    window.draw(centerLine);
}
