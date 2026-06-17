#include "Game.hpp"
#include <iostream>

Game::Game()
    : m_window(sf::VideoMode({800, 600}), "UrFu SFML 3.1 & C++23 Project")
    , m_shape(100.f)
    , m_backgroundColor(15, 23, 42) // Deep navy
{
    m_window.setFramerateLimit(60);

    // Setup the rotating shape
    m_shape.setFillColor(sf::Color(6, 182, 212)); // Cyan color
    m_shape.setOrigin({100.f, 100.f}); // Set origin to center
    m_shape.setPosition({400.f, 300.f}); // Move to window center
}

void Game::run()
{
    std::cout << "Game loop started." << std::endl;

    while (m_window.isOpen())
    {
        processEvents();
        update();
        render();
    }

    std::cout << "Game loop ended." << std::endl;
}

void Game::processEvents()
{
    while (const std::optional event = m_window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            m_window.close();
        }
        else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
        {
            if (keyPressed->code == sf::Keyboard::Key::Escape)
            {
                m_window.close();
            }
        }
    }
}

void Game::update()
{
    // Rotate the shape
    m_shape.rotate(sf::degrees(1.0f));
}

void Game::render()
{
    m_window.clear(m_backgroundColor);
    m_window.draw(m_shape);
    m_window.display();
}
