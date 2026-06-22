#include "core/Config.hpp"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <cstdint>

namespace
{
Config::ZoneType parseZoneType(const std::string& type)
{
    if (type == "spawn")
    {
        return Config::ZoneType::Spawn;
    }
    if (type == "target")
    {
        return Config::ZoneType::Target;
    }
    return Config::ZoneType::Generic;
}
}

Config Config::loadFromFile(const std::string& filepath)
{
    Config config;

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

        if (root["random_seed"])
        {
            config.randomSeed = root["random_seed"].as<unsigned int>();
        }

        if (root["lj_enabled"])
        {
            config.ljEnabled = root["lj_enabled"].as<bool>();
        }

        if (root["lj_epsilon"])
        {
            config.ljEpsilon = root["lj_epsilon"].as<float>();
        }

        if (root["lj_sigma"])
        {
            config.ljSigma = root["lj_sigma"].as<float>();
        }

        if (root["lj_cutoff"])
        {
            config.ljCutoff = root["lj_cutoff"].as<float>();
        }

        if (root["lj_max_acceleration"])
        {
            config.ljMaxAcceleration = root["lj_max_acceleration"].as<float>();
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
                config.backgroundColor.r = static_cast<std::uint8_t>(colorNode["r"].as<int>());
                config.backgroundColor.g = static_cast<std::uint8_t>(colorNode["g"].as<int>());
                config.backgroundColor.b = static_cast<std::uint8_t>(colorNode["b"].as<int>());
            }
        }

        if (root["simulation_elements"])
        {
            auto elementsNode = root["simulation_elements"];
            
            if (elementsNode["compartments"])
            {
                auto compartmentsNode = elementsNode["compartments"];
                for (const auto& compNode : compartmentsNode)
                {
                    if (compNode["name"] && compNode["polygon"])
                    {
                        Config::Compartment comp;
                        comp.name = compNode["name"].as<std::string>();
                        
                        auto polyNode = compNode["polygon"];
                        for (const auto& vertexNode : polyNode)
                        {
                            if (vertexNode["x"] && vertexNode["y"])
                            {
                                comp.polygon.push_back({
                                    vertexNode["x"].as<float>(),
                                    vertexNode["y"].as<float>()
                                });
                            }
                        }
                        config.compartments.push_back(comp);
                    }
                }
            }

            if (elementsNode["zones"])
            {
                auto zonesNode = elementsNode["zones"];
                for (const auto& zoneNode : zonesNode)
                {
                    if (zoneNode["type"] && zoneNode["compartment"])
                    {
                        Config::ZoneConfig zone;
                        zone.type = parseZoneType(zoneNode["type"].as<std::string>());
                        zone.compartment = zoneNode["compartment"].as<std::string>();
                        config.zones.push_back(zone);
                    }
                }
            }

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

            if (elementsNode["radial_fields"])
            {
                auto fieldsNode = elementsNode["radial_fields"];
                for (const auto& fieldNode : fieldsNode)
                {
                    if (fieldNode["center"] && fieldNode["intensity"])
                    {
                        auto centerNode = fieldNode["center"];
                        if (centerNode["x"] && centerNode["y"])
                        {
                            Config::RadialField field;
                            field.center = {centerNode["x"].as<float>(), centerNode["y"].as<float>()};
                            field.intensity = fieldNode["intensity"].as<float>();
                            if (fieldNode["min_radius"])
                            {
                                field.minRadius = fieldNode["min_radius"].as<float>();
                            }
                            config.radialFields.push_back(field);
                        }
                    }
                }
            }

            if (elementsNode["segment_fields"])
            {
                auto fieldsNode = elementsNode["segment_fields"];
                for (const auto& fieldNode : fieldsNode)
                {
                    if (fieldNode["start"] && fieldNode["end"] && fieldNode["intensity"])
                    {
                        auto startNode = fieldNode["start"];
                        auto endNode = fieldNode["end"];
                        if (startNode["x"] && startNode["y"] && endNode["x"] && endNode["y"])
                        {
                            Config::SegmentField field;
                            field.start = {startNode["x"].as<float>(), startNode["y"].as<float>()};
                            field.end = {endNode["x"].as<float>(), endNode["y"].as<float>()};
                            field.intensity = fieldNode["intensity"].as<float>();
                            config.segmentFields.push_back(field);
                        }
                    }
                }
            }
        }

    }
    catch (const std::exception& e)
    {
        std::cerr << "Warning: Failed to load config file '" << filepath 
                  << "' (using default settings). Error: " << e.what() << std::endl;
    }

    return config;
}
