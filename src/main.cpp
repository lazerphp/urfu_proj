#include "core/Simulation.hpp"
#include <iostream>
#include <string>
#include <cmath>

int main(int argc, char* argv[])
{
    bool headless = false;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--headless" || arg == "-h")
        {
            headless = true;
        }
        else if (arg == "--help" || arg == "-help" || arg == "-?")
        {
            std::cout << "Gas Simulation Program\n"
                      << "Usage:\n"
                      << "  " << argv[0] << " [options]\n\n"
                      << "Options:\n"
                      << "  -h, --headless    Run the simulation in headless mode (no window, output to console)\n"
                      << "  --help, -help     Show this help message\n";
            return 0;
        }
        else
        {
            std::cerr << "Unknown argument: " << arg << "\n"
                      << "Use --help to see available options." << std::endl;
            return 1;
        }
    }

    try
    {
        if (headless)
        {
            std::cout << "Initializing Application in HEADLESS mode..." << std::endl;
            SimulationCore core(Config::loadFromFile("config.yaml"));
            
            float virtualTime = 0.f;
            const float maxVirtualTime = 1000.f; // timeout in virtual seconds
            
            float lastReportTime = -1.f;
            int totalParticles = core.getParticles().size();
            
            std::cout << "Simulation loop started (Core Headless)." << std::endl;
            
            while (true)
            {
                core.update();
                virtualTime += core.getConfig().dt * core.getConfig().physicsStepsPerFrame;
                
                int reached = core.getEnvironment().getParticlesInTarget();
                
                // Output status every 1.0 virtual seconds
                if (virtualTime - lastReportTime >= 1.0f)
                {
                    std::cout << "[Virtual Time: " << std::round(virtualTime) << "s] Reached target: " 
                              << reached << "/" << totalParticles << std::endl;
                    lastReportTime = virtualTime;
                }
                
                // Target achieved condition
                if (reached >= totalParticles)
                {
                    std::cout << "Success: All " << totalParticles << " particles reached the destination reservoir!" << std::endl;
                    std::cout << "Total Virtual Time: " << virtualTime << " seconds." << std::endl;
                    break;
                }
                
                // Timeout condition
                if (virtualTime >= maxVirtualTime)
                {
                    std::cout << "Timeout: Simulation ended after " << maxVirtualTime 
                              << " virtual seconds. Particles reached: " << reached << "/" << totalParticles << std::endl;
                    break;
                }
            }
            std::cout << "Simulation loop ended." << std::endl;
        }
        else
        {
            std::cout << "Initializing Application..." << std::endl;
            Simulation sim;
            sim.run();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal Exception occurred: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
