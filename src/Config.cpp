#include "Config.hpp"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <cstdint>

Config Config::loadFromFile(const std::string& filepath)
{
    Config config; // Start with default values

    try
    {
        YAML::Node root = YAML::LoadFile(filepath);

        if (root["grid_size"])
        {
            config.gridSize = root["grid_size"].as<float>();
        }

        if (root["dt"])
        {
            config.dt = root["dt"].as<float>();
        }

        if (root["physics_steps_per_frame"])
        {
            config.physicsStepsPerFrame = root["physics_steps_per_frame"].as<int>();
        }

        if (root["particle_count"])
        {
            config.particleCount = root["particle_count"].as<int>();
        }

        if (root["particle_radius"])
        {
            config.particleRadius = root["particle_radius"].as<float>();
        }

        if (root["window"])
        {
            auto windowNode = root["window"];
            if (windowNode["width"]) config.window.width = windowNode["width"].as<unsigned int>();
            if (windowNode["height"]) config.window.height = windowNode["height"].as<unsigned int>();
            if (windowNode["title"]) config.window.title = windowNode["title"].as<std::string>();
        }

        if (root["background_color"])
        {
            auto colorNode = root["background_color"];
            if (colorNode["r"] && colorNode["g"] && colorNode["b"])
            {
                config.backgroundColor = sf::Color(
                    static_cast<std::uint8_t>(colorNode["r"].as<int>()),
                    static_cast<std::uint8_t>(colorNode["g"].as<int>()),
                    static_cast<std::uint8_t>(colorNode["b"].as<int>())
                );
            }
        }

        // Parse simulation elements (boundary polygon and obstacles)
        if (root["simulation_elements"])
        {
            auto elementsNode = root["simulation_elements"];
            
            // 1. Boundary Polygon vertices
            if (elementsNode["boundary_polygon"])
            {
                auto polyNode = elementsNode["boundary_polygon"];
                for (const auto& vertexNode : polyNode)
                {
                    if (vertexNode["x"] && vertexNode["y"])
                    {
                        config.boundaryPolygon.push_back({
                            vertexNode["x"].as<float>(),
                            vertexNode["y"].as<float>()
                        });
                    }
                }
            }

            // 2. Obstacles (independent wall segments)
            if (elementsNode["obstacles"])
            {
                auto obstaclesNode = elementsNode["obstacles"];
                for (const auto& obsNode : obstaclesNode)
                {
                    if (obsNode["start"] && obsNode["end"])
                    {
                        auto startNode = obsNode["start"];
                        auto endNode = obsNode["end"];
                        if (startNode["x"] && startNode["y"] && endNode["x"] && endNode["y"])
                        {
                            config.obstacles.push_back({
                                {startNode["x"].as<float>(), startNode["y"].as<float>()},
                                {endNode["x"].as<float>(), endNode["y"].as<float>()}
                            });
                        }
                    }
                }
            }

            // 3. Spawn Zone
            if (elementsNode["spawn_zone"])
            {
                auto zoneNode = elementsNode["spawn_zone"];
                if (zoneNode["x"] && zoneNode["y"] && zoneNode["width"] && zoneNode["height"])
                {
                    config.spawnZone.x = zoneNode["x"].as<float>();
                    config.spawnZone.y = zoneNode["y"].as<float>();
                    config.spawnZone.width = zoneNode["width"].as<float>();
                    config.spawnZone.height = zoneNode["height"].as<float>();
                    config.spawnZone.enabled = true;
                }
            }
        }

        std::cout << "Successfully loaded configuration from: " << filepath << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Warning: Failed to load config file '" << filepath 
                  << "' (using default settings). Error: " << e.what() << std::endl;
    }

    return config;
}
