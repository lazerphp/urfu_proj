#pragma once
#include "core/Types.hpp"
#include <string>
#include <vector>

struct Config
{
    float gridSize{10.f};

    // Simulation timing settings
    float dt{0.01f};
    int physicsStepsPerFrame{1};

    // Particle settings
    int particleCount{100};
    float particleRadius{3.f};

    // Lennard-Jones (6-12) Potential settings
    bool ljEnabled{true};
    float ljEpsilon{50.f};
    float ljSigma{6.f};
    float ljCutoff{15.f};
    float ljMaxAcceleration{20000.f};
    
    struct Window
    {
        unsigned int width{800};
        unsigned int height{600};
        std::string title{"Physics Simulation"};
    } window;
    
    struct BackgroundColor
    {
        unsigned char r{12};
        unsigned char g{14};
        unsigned char b{18};
    } backgroundColor;

    // Simulation elements from YAML
    struct Compartment
    {
        std::string name;
        std::vector<Vector2f> polygon;
    };
    std::vector<Compartment> compartments;

    struct ZoneConfig
    {
        std::string type;
        std::string compartment;
    };
    std::vector<ZoneConfig> zones;

    struct ObstacleSegment
    {
        Vector2f start;
        Vector2f end;
    };
    std::vector<ObstacleSegment> obstacles; // Internal independent walls

    struct RadialField
    {
        Vector2f center;
        float intensity{0.f};
        float minRadius{5.f};
    };
    std::vector<RadialField> radialFields;

    struct SegmentField
    {
        Vector2f start;
        Vector2f end;
        float intensity{0.f};
    };
    std::vector<SegmentField> segmentFields;

    // Static helper to load from file (returns defaults on failure)
    static Config loadFromFile(const std::string& filepath);
};
