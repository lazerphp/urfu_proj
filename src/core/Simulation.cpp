#include "core/Simulation.hpp"
#include "physics/Physics.hpp"
#include <iostream>
#include <random>
#include <cmath>

// ==========================================
// SimulationCore Implementation (Pure Physics)
// ==========================================

SimulationCore::SimulationCore(const Config& config)
    : m_config(config)
    , m_environment(m_config)
{
    initParticles();
}

void SimulationCore::update()
{
    for (int i = 0; i < m_config.physicsStepsPerFrame; ++i)
    {
        updatePhysics(m_config.dt);
    }

    m_environment.update(m_particles);
}

void SimulationCore::updatePhysics(float dt)
{
    for (auto& p : m_particles)
    {
        p.update(dt);

        for (const auto& wall : m_environment.getBoundarySegments())
        {
            Physics::resolveCollisionWithSegment(p, wall.start, wall.end, m_config.particleRadius, true, m_config.compartments);
        }

        for (const auto& obs : m_environment.getObstacles())
        {
            Physics::resolveCollisionWithSegment(p, obs.start, obs.end, m_config.particleRadius, false, m_config.compartments);
        }
    }
}

void SimulationCore::initParticles()
{
    const Zone* spawnZone = m_environment.getSpawnZone();
    if (!spawnZone || !spawnZone->getCompartment() || spawnZone->getCompartment()->polygon.empty())
    {
        std::cerr << "Warning: Spawn zone is empty or not found. No particles spawned." << std::endl;
        return;
    }

    const auto* comp = spawnZone->getCompartment();
    float minX = comp->polygon[0].x;
    float maxX = comp->polygon[0].x;
    float minY = comp->polygon[0].y;
    float maxY = comp->polygon[0].y;

    for (const auto& vertex : comp->polygon)
    {
        minX = std::min(minX, vertex.x);
        maxX = std::max(maxX, vertex.x);
        minY = std::min(minY, vertex.y);
        maxY = std::max(maxY, vertex.y);
    }

    float padding = m_config.particleRadius + 5.f;
    if (maxX - minX > 2.f * padding)
    {
        minX += padding;
        maxX -= padding;
    }
    if (maxY - minY > 2.f * padding)
    {
        minY += padding;
        maxY -= padding;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distX(minX, maxX);
    std::uniform_real_distribution<float> distY(minY, maxY);
    std::uniform_real_distribution<float> distSpeed(120.f, 220.f);
    std::uniform_real_distribution<float> distAngle(0.f, 2.f * 3.14159265f);

    m_particles.clear();
    m_particles.reserve(m_config.particleCount);

    int attempts = 0;
    int maxAttempts = m_config.particleCount * 200;

    while (m_particles.size() < static_cast<size_t>(m_config.particleCount) && attempts < maxAttempts)
    {
        attempts++;
        
        sf::Vector2f pos(distX(gen), distY(gen));

        if (Physics::isPointInPolygon(pos, comp->polygon))
        {
            bool insideObstacle = false;
            for (const auto& obs : m_environment.getObstacles())
            {
                sf::Vector2f closest = Physics::closestPointOnSegment(pos, obs.start, obs.end);
                sf::Vector2f diff = pos - closest;
                float distSq = diff.x * diff.x + diff.y * diff.y;
                if (distSq < m_config.particleRadius * m_config.particleRadius)
                {
                    insideObstacle = true;
                    break;
                }
            }

            if (!insideObstacle)
            {
                float speed = distSpeed(gen);
                float angle = distAngle(gen);
                sf::Vector2f vel(speed * std::cos(angle), speed * std::sin(angle));

                m_particles.emplace_back(pos, vel, m_config.particleRadius, sf::Color(6, 182, 212));
            }
        }
    }

    std::cout << "Spawned " << m_particles.size() << " particles inside compartment: " 
              << comp->name << std::endl;
}


// ==========================================
// Simulation Implementation (GUI Wrapper)
// ==========================================

Simulation::Simulation()
    : m_core(Config::loadFromFile("config.yaml"))
{
    const auto& config = m_core.getConfig();

    // Initialize the window dynamically based on config
    m_window.create(sf::VideoMode({config.window.width, config.window.height}), config.window.title);
    m_window.setFramerateLimit(60);

    // Initialize the grid cursor preview (matching config.gridSize)
    m_gridCursorPreview.setSize({config.gridSize, config.gridSize});
    m_gridCursorPreview.setFillColor(sf::Color(16, 185, 129, 80)); // Emerald green, semi-transparent
    m_gridCursorPreview.setOutlineColor(sf::Color(16, 185, 129, 200));
    m_gridCursorPreview.setOutlineThickness(1.f);
}

void Simulation::run()
{
    std::cout << "Simulation loop started." << std::endl;

    while (m_window.isOpen())
    {
        processEvents();
        update();
        render();
    }

    std::cout << "Simulation loop ended." << std::endl;
}

void Simulation::processEvents()
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
        else if (const auto* resizeEvent = event->getIf<sf::Event::Resized>())
        {
            sf::View view = m_window.getView();
            view.setSize({static_cast<float>(resizeEvent->size.x), static_cast<float>(resizeEvent->size.y)});
            m_window.setView(view);
        }
        else if (const auto* mouseButtonPressed = event->getIf<sf::Event::MouseButtonPressed>())
        {
            if (mouseButtonPressed->button == sf::Mouse::Button::Left)
            {
                m_isDragging = true;
                m_lastMousePosition = mouseButtonPressed->position;
            }
        }
        else if (const auto* mouseButtonReleased = event->getIf<sf::Event::MouseButtonReleased>())
        {
            if (mouseButtonReleased->button == sf::Mouse::Button::Left)
            {
                m_isDragging = false;
            }
        }
        else if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>())
        {
            if (m_isDragging)
            {
                sf::View view = m_window.getView();
                sf::Vector2f oldCoords = m_window.mapPixelToCoords(m_lastMousePosition);
                sf::Vector2f newCoords = m_window.mapPixelToCoords(mouseMoved->position);
                sf::Vector2f delta = oldCoords - newCoords;
                
                view.move(delta);
                m_window.setView(view);
                m_lastMousePosition = mouseMoved->position;
            }
            updateGridCursor(mouseMoved->position);
        }
        else if (const auto* mouseWheelEvent = event->getIf<sf::Event::MouseWheelScrolled>())
        {
            if (mouseWheelEvent->wheel == sf::Mouse::Wheel::Vertical)
            {
                sf::View view = m_window.getView();
                float zoomFactor = (mouseWheelEvent->delta > 0.f) ? 0.9f : 1.1f;
                sf::Vector2i mousePos = mouseWheelEvent->position;
                sf::Vector2f beforeZoom = m_window.mapPixelToCoords(mousePos);
                
                view.zoom(zoomFactor);
                m_window.setView(view);
                
                sf::Vector2f afterZoom = m_window.mapPixelToCoords(mousePos);
                sf::Vector2f offset = beforeZoom - afterZoom;
                
                view.move(offset);
                m_window.setView(view);
                
                updateGridCursor(mouseWheelEvent->position);
            }
        }
    }
}

void Simulation::update()
{
    m_core.update();

    const auto& config = m_core.getConfig();
    const auto& env = m_core.getEnvironment();
    const auto& particles = m_core.getParticles();

    int targetCount = env.getParticlesInTarget();
    static int lastTargetCount = -1;
    if (targetCount != lastTargetCount)
    {
        std::string title = config.window.title + " | Particles Reached: " + std::to_string(targetCount) + "/" + std::to_string(particles.size());
        m_window.setTitle(title);
        lastTargetCount = targetCount;
    }
}

void Simulation::render()
{
    m_window.clear(m_core.getConfig().backgroundColor);
    
    drawGrid();

    m_core.getEnvironment().draw(m_window);

    for (const auto& p : m_core.getParticles())
    {
        p.draw(m_window);
    }

    m_window.draw(m_gridCursorPreview);
    
    m_window.display();
}

sf::Vector2f Simulation::snapToGrid(sf::Vector2f position) const
{
    float gridSize = m_core.getConfig().gridSize;
    float x = std::floor(position.x / gridSize) * gridSize;
    float y = std::floor(position.y / gridSize) * gridSize;
    return {x, y};
}

void Simulation::drawGrid()
{
    sf::View view = m_window.getView();
    sf::Vector2f center = view.getCenter();
    sf::Vector2f size = view.getSize();

    float left = center.x - size.x / 2.f;
    float right = center.x + size.x / 2.f;
    float top = center.y - size.y / 2.f;
    float bottom = center.y + size.y / 2.f;

    float gridSize = m_core.getConfig().gridSize;
    float screenCellSize = gridSize * (m_window.getSize().x / size.x);
    if (screenCellSize < 4.f)
    {
        gridSize *= 10.f;
    }

    float startX = std::floor(left / gridSize) * gridSize;
    float endX = std::ceil(right / gridSize) * gridSize;
    float startY = std::floor(top / gridSize) * gridSize;
    float endY = std::ceil(bottom / gridSize) * gridSize;

    sf::VertexArray lines(sf::PrimitiveType::Lines);
    sf::Color gridColor(50, 60, 80);

    for (float x = startX; x <= endX; x += gridSize)
    {
        if (std::abs(x) < 0.001f) continue;
        lines.append(sf::Vertex({x, top}, gridColor));
        lines.append(sf::Vertex({x, bottom}, gridColor));
    }

    for (float y = startY; y <= endY; y += gridSize)
    {
        if (std::abs(y) < 0.001f) continue;
        lines.append(sf::Vertex({left, y}, gridColor));
        lines.append(sf::Vertex({right, y}, gridColor));
    }

    m_window.draw(lines);

    sf::VertexArray axes(sf::PrimitiveType::Lines);
    sf::Color axisColor(148, 163, 184);

    if (top <= 0.f && bottom >= 0.f)
    {
        axes.append(sf::Vertex({left, 0.f}, axisColor));
        axes.append(sf::Vertex({right, 0.f}, axisColor));
    }

    if (left <= 0.f && right >= 0.f)
    {
        axes.append(sf::Vertex({0.f, top}, axisColor));
        axes.append(sf::Vertex({0.f, bottom}, axisColor));
    }

    m_window.draw(axes);
}

void Simulation::updateGridCursor(sf::Vector2i mousePosition)
{
    sf::Vector2f worldPos = m_window.mapPixelToCoords(mousePosition);
    sf::Vector2f snappedPos = snapToGrid(worldPos);
    m_gridCursorPreview.setPosition(snappedPos);
}
