#include "core/Simulation.hpp"
#include "physics/Physics.hpp"
#include <iostream>
#include <random>
#include <cmath>

namespace {
    sf::Vector2f toSF(const Vector2f& v) {
        return {v.x, v.y};
    }
}

// ==========================================
// Simulation Implementation (Pure Physics Core)
// ==========================================

Simulation::Simulation()
    : m_config(Config::loadFromFile("config.yaml"))
    , m_environment(m_config)
{
    initParticles();
    m_targetZone = m_environment.getTargetZone();
    m_reachedCount = m_targetZone ? m_targetZone->getParticleCount() : 0;
}

void Simulation::run()
{
    std::cout << "Simulation loop started (Headless)." << std::endl;
    
    while (!isFinished())
    {
        step();
        reportStatus();
    }
    
    reportResult();
    std::cout << "Simulation loop ended." << std::endl;
}

bool Simulation::isFinished() const
{
    const float maxVirtualTime = 1000.f;
    return m_reachedCount >= m_config.particleCount || m_virtualTime >= maxVirtualTime;
}

bool Simulation::isSuccess() const
{
    return m_reachedCount >= m_config.particleCount;
}

void Simulation::step()
{
    update();
    m_virtualTime += m_config.dt * m_config.physicsStepsPerFrame;
    m_reachedCount = m_targetZone ? m_targetZone->getParticleCount() : 0;
}

void Simulation::reportStatus()
{
    if (m_virtualTime - m_lastReportTime >= 1.0f)
    {
        std::cout << "[Virtual Time: " << std::round(m_virtualTime) << "s] Reached target: " 
                  << m_reachedCount << "/" << m_config.particleCount << std::endl;
        m_lastReportTime = m_virtualTime;
    }
}

void Simulation::reportResult() const
{
    if (isSuccess())
    {
        std::cout << "Success: All " << m_config.particleCount << " particles reached the destination reservoir!" << std::endl;
        std::cout << "Total Virtual Time: " << m_virtualTime << " seconds." << std::endl;
    }
    else
    {
        std::cout << "Timeout: Simulation ended after " << m_virtualTime 
                  << " virtual seconds. Particles reached: " << m_reachedCount << "/" << m_config.particleCount << std::endl;
    }
}

void Simulation::update()
{
    for (int i = 0; i < m_config.physicsStepsPerFrame; ++i)
    {
        updatePhysics(m_config.dt);
    }

    // Update zones
    for (auto& zone : m_environment.getZones())
    {
        zone->processParticles(m_particles);
    }
}

void Simulation::updatePhysics(float dt)
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

        for (const auto& zone : m_environment.getZones())
        {
            zone->resolvePhysics(p, m_config.particleRadius);
        }
    }
}

void Simulation::initParticles()
{
    const Zone* spawnZone = nullptr;
    for (const auto& zone : m_environment.getZones())
    {
        if (zone->getType() == "spawn")
        {
            spawnZone = zone.get();
            break;
        }
    }

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
        
        Vector2f pos(distX(gen), distY(gen));

        if (Physics::isPointInPolygon(pos, comp->polygon))
        {
            bool insideObstacle = false;
            for (const auto& obs : m_environment.getObstacles())
            {
                Vector2f closest = Physics::closestPointOnSegment(pos, obs.start, obs.end);
                Vector2f diff = pos - closest;
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
                Vector2f vel(speed * std::cos(angle), speed * std::sin(angle));

                m_particles.emplace_back(pos, vel, m_config.particleRadius);
            }
        }
    }

    std::cout << "Spawned " << m_particles.size() << " particles inside compartment: " 
              << comp->name << std::endl;
}


// ==========================================
// Visualizer Implementation (SFML Interface)
// ==========================================

Visualizer::Visualizer(Simulation& sim)
    : m_sim(sim)
{
    const auto& config = m_sim.getConfig();

    // Initialize the window dynamically based on config
    m_window.create(sf::VideoMode({config.window.width, config.window.height}), config.window.title);
    m_window.setFramerateLimit(60);

    // Initialize the grid cursor preview (matching config.gridSize)
    m_gridCursorPreview.setSize({config.gridSize, config.gridSize});
    m_gridCursorPreview.setFillColor(sf::Color(16, 185, 129, 80)); // Emerald green, semi-transparent

    // Load system font
    if (m_font.openFromFile("/usr/share/fonts/truetype/noto/NotoSansMono-Bold.ttf") ||
        m_font.openFromFile("/usr/share/fonts/truetype/noto/NotoSansMono-Regular.ttf") ||
        m_font.openFromFile("/usr/share/fonts/truetype/noto/NotoMono-Regular.ttf"))
    {
        m_fontLoaded = true;
    }
    else
    {
        std::cerr << "Warning: Could not load system Noto font. HUD will not be rendered." << std::endl;
    }
}

void Visualizer::run()
{
    std::cout << "Simulation loop started (GUI)." << std::endl;

    while (m_window.isOpen())
    {
        processEvents();
        update();
        render();
    }

    std::cout << "Simulation loop ended." << std::endl;
}

void Visualizer::processEvents()
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

void Visualizer::update()
{
    const auto& config = m_sim.getConfig();
    const auto& particles = m_sim.getParticles();

    if (!m_sim.isFinished())
    {
        m_sim.step();
        m_sim.reportStatus();
        
        if (m_sim.isFinished())
        {
            m_sim.reportResult();
        }
    }

    int targetCount = m_sim.getReachedCount();
    float time = m_sim.getVirtualTime();
    static int lastTargetCount = -1;
    static int lastTimeSec = -1;
    int timeSec = static_cast<int>(std::round(time));
    
    if (targetCount != lastTargetCount || timeSec != lastTimeSec)
    {
        std::string title = config.window.title + " | Reached: " + std::to_string(targetCount) + "/" + std::to_string(particles.size()) + " | Time: " + std::to_string(timeSec) + "s";
        m_window.setTitle(title);
        lastTargetCount = targetCount;
        lastTimeSec = timeSec;
    }
}

void Visualizer::render()
{
    const auto& bg = m_sim.getConfig().backgroundColor;
    m_window.clear(sf::Color(bg.r, bg.g, bg.b));
    
    drawGrid();

    // Draw Zones
    for (const auto& zone : m_sim.getEnvironment().getZones())
    {
        if (zone->getCompartment() && !zone->getCompartment()->polygon.empty())
        {
            sf::ConvexShape shape(zone->getCompartment()->polygon.size());
            for (size_t i = 0; i < zone->getCompartment()->polygon.size(); ++i)
            {
                shape.setPoint(i, toSF(zone->getCompartment()->polygon[i]));
            }

            sf::Color fillColor, outlineColor;
            if (zone->getType() == "spawn")
            {
                fillColor = sf::Color(59, 130, 246, 40);
                outlineColor = sf::Color(59, 130, 246, 150);
            }
            else if (zone->getType() == "target")
            {
                fillColor = sf::Color(16, 185, 129, 40);
                outlineColor = sf::Color(16, 185, 129, 150);
            }
            else
            {
                fillColor = sf::Color(148, 163, 184, 40);
                outlineColor = sf::Color(148, 163, 184, 150);
            }

            shape.setFillColor(fillColor);
            shape.setOutlineColor(outlineColor);
            shape.setOutlineThickness(1.f);
            m_window.draw(shape);
        }
    }

    // Draw Obstacles
    if (!m_sim.getEnvironment().getObstacles().empty())
    {
        sf::VertexArray wallLines(sf::PrimitiveType::Lines);
        for (const auto& wall : m_sim.getEnvironment().getObstacles())
        {
            wallLines.append(sf::Vertex(toSF(wall.start), sf::Color::White));
            wallLines.append(sf::Vertex(toSF(wall.end), sf::Color::White));
        }
        m_window.draw(wallLines);
    }

    // Draw Outer Boundary Walls
    if (!m_sim.getEnvironment().getBoundarySegments().empty())
    {
        sf::VertexArray polyLines(sf::PrimitiveType::Lines);
        for (const auto& seg : m_sim.getEnvironment().getBoundarySegments())
        {
            polyLines.append(sf::Vertex(toSF(seg.start), sf::Color::White));
            polyLines.append(sf::Vertex(toSF(seg.end), sf::Color::White));
        }
        m_window.draw(polyLines);
    }

    // Draw Particles
    for (const auto& p : m_sim.getParticles())
    {
        sf::CircleShape circle(p.getRadius());
        circle.setPosition(toSF(p.getPosition()));
        circle.setOrigin({p.getRadius(), p.getRadius()});
        circle.setFillColor(sf::Color(6, 182, 212)); // Fixed cyan color
        m_window.draw(circle);
    }

    m_window.draw(m_gridCursorPreview);

    // Draw HUD banner (anchored to top-right corner of the window)
    if (m_fontLoaded)
    {
        sf::View currentView = m_window.getView();
        m_window.setView(m_window.getDefaultView());

        // Get window size
        sf::Vector2u winSize = m_window.getSize();
        float bannerWidth = 100.f;
        float bannerHeight = 35.f;
        float padding = 20.f;

        float x = winSize.x - bannerWidth - padding;
        float y = padding;

        // Background banner shape (dark translucent panel)
        sf::RectangleShape banner({bannerWidth, bannerHeight});
        banner.setPosition({x, y});
        banner.setFillColor(sf::Color(17, 24, 39, 210)); // Sleek dark gray
        banner.setOutlineColor(sf::Color(16, 185, 129, 220)); // Emerald border
        banner.setOutlineThickness(1.5f);

        // Reached count text (e.g., 49/50)
        sf::Text hudText(m_font);
        int reached = m_sim.getReachedCount();
        int total = m_sim.getParticles().size();
        hudText.setString(std::to_string(reached) + "/" + std::to_string(total));
        hudText.setCharacterSize(16);
        hudText.setFillColor(sf::Color::White);
        
        // Center text inside the banner
        sf::FloatRect bounds = hudText.getLocalBounds();
        hudText.setOrigin({bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f});
        hudText.setPosition({x + bannerWidth / 2.f, y + bannerHeight / 2.f});

        m_window.draw(banner);
        m_window.draw(hudText);

        m_window.setView(currentView);
    }
    
    m_window.display();
}

sf::Vector2f Visualizer::snapToGrid(sf::Vector2f position) const
{
    float gridSize = m_sim.getConfig().gridSize;
    float x = std::floor(position.x / gridSize) * gridSize;
    float y = std::floor(position.y / gridSize) * gridSize;
    return {x, y};
}

void Visualizer::drawGrid()
{
    sf::View view = m_window.getView();
    sf::Vector2f center = view.getCenter();
    sf::Vector2f size = view.getSize();

    float left = center.x - size.x / 2.f;
    float right = center.x + size.x / 2.f;
    float top = center.y - size.y / 2.f;
    float bottom = center.y + size.y / 2.f;

    float gridSize = m_sim.getConfig().gridSize;
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

void Visualizer::updateGridCursor(sf::Vector2i mousePosition)
{
    sf::Vector2f worldPos = m_window.mapPixelToCoords(mousePosition);
    sf::Vector2f snappedPos = snapToGrid(worldPos);
    m_gridCursorPreview.setPosition(snappedPos);
}
