#include "Game.hpp"
#include <iostream>

int main()
{
    std::cout << "Initializing Application..." << std::endl;
    try
    {
        Game game;
        game.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Application crashed with exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Application crashed with unknown error." << std::endl;
        return 1;
    }
    return 0;
}
