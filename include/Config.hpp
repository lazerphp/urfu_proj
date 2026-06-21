#pragma once
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
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
    
    struct Window
    {
        unsigned int width{800};
        unsigned int height{600};
        std::string title{"Physics Simulation"};
    } window;
    
    sf::Color backgroundColor{15, 23, 42};

    // Simulation elements from YAML
    struct Compartment
    {
        std::string name;
        std::vector<sf::Vector2f> polygon;
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
        sf::Vector2f start;
        sf::Vector2f end;
    };
    std::vector<ObstacleSegment> obstacles; // Internal independent walls

    // Static helper to load from file (returns defaults on failure)
    static Config loadFromFile(const std::string& filepath);
};
