#include "Game.hpp"
#include <iostream>
#include <random>
#include <cmath>

Game::Game()
    : m_config(Config::loadFromFile("config.yaml"))
    , m_spawnZone(m_config.spawnZone.x, m_config.spawnZone.y, m_config.spawnZone.width, m_config.spawnZone.height, m_config.spawnZone.enabled)
{
    // Initialize the window dynamically based on config
    m_window.create(sf::VideoMode({m_config.window.width, m_config.window.height}), m_config.window.title);
    m_window.setFramerateLimit(60);

    // Initialize the grid cursor preview (matching config.gridSize)
    m_gridCursorPreview.setSize({m_config.gridSize, m_config.gridSize});
    m_gridCursorPreview.setFillColor(sf::Color(16, 185, 129, 80)); // Emerald green, semi-transparent
    m_gridCursorPreview.setOutlineColor(sf::Color(16, 185, 129, 200));
    m_gridCursorPreview.setOutlineThickness(1.f);
    m_gridCursorPreview.setOrigin({m_config.gridSize / 2.f, m_config.gridSize / 2.f});

    initParticles();
}

void Game::run()
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
        else if (const auto* resizeEvent = event->getIf<sf::Event::Resized>())
        {
            // Get current view
            sf::View view = m_window.getView();
            // Update its size to match the new window dimensions
            view.setSize({static_cast<float>(resizeEvent->size.x), static_cast<float>(resizeEvent->size.y)});
            // Apply the updated view back to the window
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
                // Convert screen mouse movement to world coordinate movement
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
                
                // Determine zoom factor (scrolling up zooms in, down zooms out)
                float zoomFactor = (mouseWheelEvent->delta > 0.f) ? 0.9f : 1.1f;
                
                // Zoom towards the cursor position:
                sf::Vector2i mousePos = mouseWheelEvent->position;
                sf::Vector2f beforeZoom = m_window.mapPixelToCoords(mousePos);
                
                view.zoom(zoomFactor);
                m_window.setView(view); // Update window view so mapPixelToCoords uses the zoomed view
                
                sf::Vector2f afterZoom = m_window.mapPixelToCoords(mousePos);
                sf::Vector2f offset = beforeZoom - afterZoom;
                
                view.move(offset);
                m_window.setView(view);
                
                updateGridCursor(mouseWheelEvent->position);
            }
        }
    }
}

void Game::update()
{
    // Perform a fixed number of physics steps per frame
    for (int i = 0; i < m_config.physicsStepsPerFrame; ++i)
    {
        updatePhysics(m_config.dt);
    }
}

void Game::updatePhysics(float dt)
{
    for (auto& p : m_particles)
    {
        // 1. Move particle using its own class method
        p.update(dt);

        // 2. Collide with outer boundary corridor edges
        for (size_t i = 0; i < m_config.boundaryPolygon.size(); ++i)
        {
            sf::Vector2f a = m_config.boundaryPolygon[i];
            sf::Vector2f b = m_config.boundaryPolygon[(i + 1) % m_config.boundaryPolygon.size()];
            resolveCollisionWithSegment(p, a, b, m_config.particleRadius, true);
        }

        // 3. Collide with internal obstacle segments
        for (const auto& obs : m_config.obstacles)
        {
            resolveCollisionWithSegment(p, obs.start, obs.end, m_config.particleRadius, false);
        }
    }
}

void Game::render()
{
    m_window.clear(m_config.backgroundColor);
    
    // Draw the gray grid lines and coordinate axes
    drawGrid();

    // Draw the spawn zone using its own class method
    m_spawnZone.draw(m_window);
    
    // Draw obstacles (white lines)
    if (!m_config.obstacles.empty())
    {
        sf::VertexArray wallLines(sf::PrimitiveType::Lines);
        for (const auto& wall : m_config.obstacles)
        {
            wallLines.append(sf::Vertex(wall.start, sf::Color::White));
            wallLines.append(sf::Vertex(wall.end, sf::Color::White));
        }
        m_window.draw(wallLines);
    }

    // Draw the closed boundary polygon (white loop)
    if (!m_config.boundaryPolygon.empty())
    {
        sf::VertexArray polyLines(sf::PrimitiveType::LineStrip);
        for (const auto& vertex : m_config.boundaryPolygon)
        {
            polyLines.append(sf::Vertex(vertex, sf::Color::White));
        }
        // Close the loop
        polyLines.append(sf::Vertex(m_config.boundaryPolygon.front(), sf::Color::White));
        m_window.draw(polyLines);
    }

    // Draw particles using their own class method
    for (const auto& p : m_particles)
    {
        p.draw(m_window);
    }

    // Draw the grid cursor preview
    m_window.draw(m_gridCursorPreview);
    
    m_window.display();
}

sf::Vector2f Game::snapToGrid(sf::Vector2f position) const
{
    float x = std::round(position.x / m_config.gridSize) * m_config.gridSize;
    float y = std::round(position.y / m_config.gridSize) * m_config.gridSize;
    return {x, y};
}

void Game::drawGrid()
{
    sf::View view = m_window.getView();
    sf::Vector2f center = view.getCenter();
    sf::Vector2f size = view.getSize();

    float left = center.x - size.x / 2.f;
    float right = center.x + size.x / 2.f;
    float top = center.y - size.y / 2.f;
    float bottom = center.y + size.y / 2.f;

    // Grid size
    float gridSize = m_config.gridSize;

    // Check if the grid is too dense to render (less than 4 pixels per cell on screen)
    float screenCellSize = gridSize * (m_window.getSize().x / size.x);
    if (screenCellSize < 4.f)
    {
        // Grid is too dense, draw a coarser grid (every 10th line) to avoid solid gray block
        gridSize *= 10.f;
    }

    // Determine start and end coordinates aligned to the grid
    float startX = std::floor(left / gridSize) * gridSize;
    float endX = std::ceil(right / gridSize) * gridSize;
    float startY = std::floor(top / gridSize) * gridSize;
    float endY = std::ceil(bottom / gridSize) * gridSize;

    sf::VertexArray lines(sf::PrimitiveType::Lines);
    sf::Color gridColor(50, 60, 80); // Subtle dark blue-gray for the grid lines

    // Vertical lines
    for (float x = startX; x <= endX; x += gridSize)
    {
        if (std::abs(x) < 0.001f) continue; // Skip x = 0 to draw it as axis
        lines.append(sf::Vertex({x, top}, gridColor));
        lines.append(sf::Vertex({x, bottom}, gridColor));
    }

    // Horizontal lines
    for (float y = startY; y <= endY; y += gridSize)
    {
        if (std::abs(y) < 0.001f) continue; // Skip y = 0 to draw it as axis
        lines.append(sf::Vertex({left, y}, gridColor));
        lines.append(sf::Vertex({right, y}, gridColor));
    }

    m_window.draw(lines);

    // Draw Coordinate Axes (X = 0 and Y = 0)
    sf::VertexArray axes(sf::PrimitiveType::Lines);
    sf::Color axisColor(148, 163, 184); // Bright slate gray for the main axes

    // X-axis (horizontal line at y = 0)
    if (top <= 0.f && bottom >= 0.f)
    {
        axes.append(sf::Vertex({left, 0.f}, axisColor));
        axes.append(sf::Vertex({right, 0.f}, axisColor));
    }

    // Y-axis (vertical line at x = 0)
    if (left <= 0.f && right >= 0.f)
    {
        axes.append(sf::Vertex({0.f, top}, axisColor));
        axes.append(sf::Vertex({0.f, bottom}, axisColor));
    }

    m_window.draw(axes);
}

void Game::updateGridCursor(sf::Vector2i mousePosition)
{
    // Convert screen coordinates to world coordinates
    sf::Vector2f worldPos = m_window.mapPixelToCoords(mousePosition);
    // Snap world coordinates to grid
    sf::Vector2f snappedPos = snapToGrid(worldPos);
    // Move the preview to snapped grid coordinate
    m_gridCursorPreview.setPosition(snappedPos);
}

void Game::initParticles()
{
    if (m_config.boundaryPolygon.empty())
    {
        std::cerr << "Warning: Boundary polygon is empty. No particles spawned." << std::endl;
        return;
    }

    // Find bounding box of the boundary polygon for fallback
    float minX = m_config.boundaryPolygon[0].x;
    float maxX = m_config.boundaryPolygon[0].x;
    float minY = m_config.boundaryPolygon[0].y;
    float maxY = m_config.boundaryPolygon[0].y;

    for (const auto& vertex : m_config.boundaryPolygon)
    {
        minX = std::min(minX, vertex.x);
        maxX = std::max(maxX, vertex.x);
        minY = std::min(minY, vertex.y);
        maxY = std::max(maxY, vertex.y);
    }

    // Setup random generators
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distX(minX, maxX);
    std::uniform_real_distribution<float> distY(minY, maxY);
    std::uniform_real_distribution<float> distSpeed(120.f, 220.f); // Initial velocity speed range
    std::uniform_real_distribution<float> distAngle(0.f, 2.f * 3.14159265f); // 360 degrees

    m_particles.clear();
    m_particles.reserve(m_config.particleCount);

    int attempts = 0;
    int maxAttempts = m_config.particleCount * 200; // Prevent infinite loop

    while (m_particles.size() < static_cast<size_t>(m_config.particleCount) && attempts < maxAttempts)
    {
        attempts++;
        
        // Spawn inside spawnZone if enabled, otherwise fall back to polygon bounding box
        sf::Vector2f pos = m_spawnZone.isEnabled() 
            ? m_spawnZone.getRandomPosition(gen) 
            : sf::Vector2f(distX(gen), distY(gen));

        // Ensure the particle is spawned INSIDE the boundary polygon
        if (isPointInPolygon(pos, m_config.boundaryPolygon))
        {
            // Also check it doesn't spawn inside an obstacle to be clean
            bool insideObstacle = false;
            for (const auto& obs : m_config.obstacles)
            {
                sf::Vector2f closest = closestPointOnSegment(pos, obs.start, obs.end);
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

    std::cout << "Spawned " << m_particles.size() << " particles inside the corridor." << std::endl;
}

bool Game::isPointInPolygon(sf::Vector2f p, const std::vector<sf::Vector2f>& polygon) const
{
    int n = polygon.size();
    if (n < 3) return false;
    bool inside = false;
    for (int i = 0, j = n - 1; i < n; j = i++)
    {
        if (((polygon[i].y > p.y) != (polygon[j].y > p.y)) &&
            (p.x < (polygon[j].x - polygon[i].x) * (p.y - polygon[i].y) / (polygon[j].y - polygon[i].y) + polygon[i].x))
        {
            inside = !inside;
        }
    }
    return inside;
}

sf::Vector2f Game::closestPointOnSegment(sf::Vector2f p, sf::Vector2f a, sf::Vector2f b) const
{
    sf::Vector2f ab = b - a;
    sf::Vector2f ap = p - a;
    float abLenSq = ab.x * ab.x + ab.y * ab.y;
    if (abLenSq < 0.0001f) return a;
    
    float t = (ap.x * ab.x + ap.y * ab.y) / abLenSq;
    t = std::max(0.f, std::min(1.f, t));
    return a + t * ab;
}

void Game::resolveCollisionWithSegment(Particle& p, sf::Vector2f a, sf::Vector2f b, float radius, bool isBoundary) const
{
    sf::Vector2f pos = p.getPosition();
    sf::Vector2f vel = p.getVelocity();
    sf::Vector2f closest = closestPointOnSegment(pos, a, b);
    sf::Vector2f diff = pos - closest;
    float distSq = diff.x * diff.x + diff.y * diff.y;
    float rSq = radius * radius;
    
    if (distSq < rSq && distSq > 0.00001f)
    {
        float dist = std::sqrt(distSq);
        sf::Vector2f normal = diff / dist;
        
        if (isBoundary)
        {
            // For outer boundary edges, the normal must point INWARD (into the polygon)
            sf::Vector2f testPos = closest + normal * 0.1f;
            if (!isPointInPolygon(testPos, m_config.boundaryPolygon))
            {
                normal = -normal; // Flip it to point inward
            }
        }

        // Push the particle out of the wall to prevent sticking
        p.setPosition(closest + normal * radius);
        
        // Reflect velocity vector: v_new = v - 2 * (v . n) * n
        float vn = vel.x * normal.x + vel.y * normal.y;
        if (vn < 0.f) // Only reflect if moving towards the wall segment
        {
            p.setVelocity(vel - 2.f * vn * normal);
        }
    }
}

