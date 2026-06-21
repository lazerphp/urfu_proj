#include "Game.hpp"
#include <iostream>
#include <random>
#include <cmath>

Game::Game()
    : m_config(Config::loadFromFile("config.yaml"))
{
    buildBoundarySegments();

    // Initialize the window dynamically based on config
    m_window.create(sf::VideoMode({m_config.window.width, m_config.window.height}), m_config.window.title);
    m_window.setFramerateLimit(60);

    // Initialize the grid cursor preview (matching config.gridSize)
    m_gridCursorPreview.setSize({m_config.gridSize, m_config.gridSize});
    m_gridCursorPreview.setFillColor(sf::Color(16, 185, 129, 80)); // Emerald green, semi-transparent
    m_gridCursorPreview.setOutlineColor(sf::Color(16, 185, 129, 200));
    m_gridCursorPreview.setOutlineThickness(1.f);

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

    // Update zones (including particle counts)
    int targetCount = 0;
    for (auto& zone : m_zones)
    {
        zone.update(m_particles);
        if (zone.getType() == "target")
        {
            targetCount = zone.getParticleCount();
        }
    }

    // Update window title only when the value changes (prevents Linux X11 server lag)
    static int lastTargetCount = -1;
    if (targetCount != lastTargetCount)
    {
        std::string title = m_config.window.title + " | Particles Reached: " + std::to_string(targetCount) + "/" + std::to_string(m_particles.size());
        m_window.setTitle(title);
        lastTargetCount = targetCount;
    }
}

void Game::updatePhysics(float dt)
{
    for (auto& p : m_particles)
    {
        // 1. Move particle using its own class method
        p.update(dt);

        // 2. Collide with outer boundary corridor edges
        for (const auto& wall : m_boundarySegments)
        {
            resolveCollisionWithSegment(p, wall.start, wall.end, m_config.particleRadius, true);
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

    // Draw all active logical zones (spawn, target, etc.)
    for (const auto& zone : m_zones)
    {
        zone.draw(m_window);
    }
    
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

    // Draw the outer boundary segments (white lines)
    if (!m_boundarySegments.empty())
    {
        sf::VertexArray polyLines(sf::PrimitiveType::Lines);
        for (const auto& seg : m_boundarySegments)
        {
            polyLines.append(sf::Vertex(seg.start, sf::Color::White));
            polyLines.append(sf::Vertex(seg.end, sf::Color::White));
        }
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
    float x = std::floor(position.x / m_config.gridSize) * m_config.gridSize;
    float y = std::floor(position.y / m_config.gridSize) * m_config.gridSize;
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
    if (!m_spawnZonePtr || !m_spawnZonePtr->getCompartment() || m_spawnZonePtr->getCompartment()->polygon.empty())
    {
        std::cerr << "Warning: Spawn zone or compartment is empty or not found. No particles spawned." << std::endl;
        return;
    }

    const auto* comp = m_spawnZonePtr->getCompartment();
    // Find bounding box of the spawn compartment polygon
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

    // Apply padding of particle_radius + 5.0f to avoid spawning inside walls
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
        
        sf::Vector2f pos(distX(gen), distY(gen));

        // Ensure the particle is spawned INSIDE the spawn compartment polygon
        if (isPointInPolygon(pos, comp->polygon))
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

    std::cout << "Spawned " << m_particles.size() << " particles inside compartment: " 
              << comp->name << std::endl;
}

void Game::buildBoundarySegments()
{
    m_boundarySegments.clear();
    m_zones.clear();
    m_spawnZonePtr = nullptr;

    // 1. Instantiate zones from config
    for (const auto& zoneConf : m_config.zones)
    {
        const Config::Compartment* matchedComp = nullptr;
        for (const auto& comp : m_config.compartments)
        {
            if (comp.name == zoneConf.compartment)
            {
                matchedComp = &comp;
                break;
            }
        }
        if (matchedComp)
        {
            m_zones.emplace_back(zoneConf.type, matchedComp);
        }
    }

    // Identify spawn zone pointer
    for (auto& zone : m_zones)
    {
        if (zone.getType() == "spawn")
        {
            m_spawnZonePtr = &zone;
        }
    }

    // 2. Gather all segments from all compartments
    struct SegmentInfo {
        sf::Vector2f start;
        sf::Vector2f end;
        bool isShared = false;
    };
    std::vector<SegmentInfo> allSegments;

    for (const auto& comp : m_config.compartments)
    {
        size_t n = comp.polygon.size();
        for (size_t i = 0; i < n; ++i)
        {
            allSegments.push_back({
                comp.polygon[i],
                comp.polygon[(i + 1) % n],
                false
            });
        }
    }

    // 3. Mark shared segments (passages between compartments)
    auto approxEqual = [](sf::Vector2f p1, sf::Vector2f p2) {
        float dx = p1.x - p2.x;
        float dy = p1.y - p2.y;
        return (dx * dx + dy * dy) < 0.1f;
    };

    for (size_t i = 0; i < allSegments.size(); ++i)
    {
        for (size_t j = i + 1; j < allSegments.size(); ++j)
        {
            bool matchNormal = approxEqual(allSegments[i].start, allSegments[j].start) && 
                              approxEqual(allSegments[i].end, allSegments[j].end);
            bool matchReverse = approxEqual(allSegments[i].start, allSegments[j].end) && 
                               approxEqual(allSegments[i].end, allSegments[j].start);
            if (matchNormal || matchReverse)
            {
                allSegments[i].isShared = true;
                allSegments[j].isShared = true;
            }
        }
    }

    // 4. Save non-shared segments as outer boundaries
    for (const auto& seg : allSegments)
    {
        if (!seg.isShared)
        {
            m_boundarySegments.push_back({seg.start, seg.end});
        }
    }
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

bool Game::isPointInCorridor(sf::Vector2f p) const
{
    for (const auto& comp : m_config.compartments)
    {
        if (isPointInPolygon(p, comp.polygon))
        {
            return true;
        }
    }
    return false;
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
            // For outer boundary edges, the normal must point INWARD (into the corridor)
            sf::Vector2f testPos = closest + normal * 0.1f;
            if (!isPointInCorridor(testPos))
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

