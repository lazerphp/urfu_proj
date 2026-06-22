#include "core/Simulation.hpp"
#include "graphics/Visualizer.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    bool headless = false;
    std::string configPath = "config.yaml";

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--headless" || arg == "-h")
        {
            headless = true;
        }
        else if ((arg == "--config" || arg == "-c") && i + 1 < argc)
        {
            configPath = argv[++i];
        }
        else if (arg == "--help" || arg == "-help" || arg == "-?")
        {
            std::cout << "Gas Simulation Program\n"
                      << "Usage:\n"
                      << "  " << argv[0] << " [options]\n\n"
                      << "Options:\n"
                      << "  -h, --headless            Run the simulation in headless mode (no window, output to console)\n"
                      << "  -c, --config <filepath>   Path to configuration yaml file (default: config.yaml)\n"
                      << "  --help, -help             Show this help message\n";
            return 0;
        }
        else
        {
            std::cerr << "Unknown or invalid argument: " << arg << "\n"
                      << "Use --help to see available options." << std::endl;
            return 1;
        }
    }

    try
    {
        Simulation sim(configPath);
        if (headless)
        {
            sim.run();
        }
        else
        {
            std::cout << "Initializing Application..." << std::endl;
            Visualizer vis(sim);
            vis.run();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal Exception occurred: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
