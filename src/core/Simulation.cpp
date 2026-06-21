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
                fillColor = sf::Color(150, 210, 40, 40);
                outlineColor = sf::Color(150, 210, 40, 150);
            }
            else if (zone->getType() == "target")
            {
                fillColor = sf::Color(220, 50, 50, 40);
                outlineColor = sf::Color(220, 50, 50, 150);
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
        for (const auto& wall : m_sim.getEnvironment().getObstacles())
        {
            sf::Vector2f start = toSF(wall.start);
            sf::Vector2f end = toSF(wall.end);
            sf::Vector2f dir = end - start;
            float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            if (length < 0.001f) continue;

            sf::RectangleShape lineShape(sf::Vector2f(length, 4.0f));
            lineShape.setOrigin({0.f, 2.0f});
            lineShape.setPosition(start);
            lineShape.setRotation(sf::radians(std::atan2(dir.y, dir.x)));
            lineShape.setFillColor(sf::Color::White);
            m_window.draw(lineShape);
        }
    }
    drawGrid();

    // Draw Radial Fields (Point Sources) with smooth concentric-band volumetric glow
    for (const auto& field : m_sim.getConfig().radialFields)
    {
        // Ambient reach: larger spread but very pale/low opacity (maxAlpha 110.f) for subtle decay
        float maxRadius = std::min(250.f, std::sqrt(std::abs(field.intensity)) * 3.5f);
        if (maxRadius < 10.f) maxRadius = 70.f;

        // Custom colors: attractive amber/yellow or repulsive sky-blue
        sf::Color baseColor = (field.intensity < 0.f) ? sf::Color(255, 180, 70) : sf::Color(80, 190, 255);

        // Smooth bell-curve function: f(x) = (1 - x^2)^2. Samples boundaries to create seamless transition bands
        const int numBands = 8;
        float x[numBands] = { 0.0f, 0.15f, 0.3f, 0.45f, 0.6f, 0.75f, 0.9f, 1.0f };
        float alphaVal[numBands];
        float maxAlpha = 110.f;
        for (int i = 0; i < numBands; ++i)
        {
            float val = 1.f - x[i] * x[i];
            alphaVal[i] = maxAlpha * val * val;
        }

        const int numSegments = 32;
        sf::Vector2f center = toSF(field.center);

        // 1. Center disc (from radius 0 to x[1]*maxRadius)
        {
            sf::Color c0 = baseColor; c0.a = static_cast<unsigned char>(alphaVal[0]);
            sf::Color c1 = baseColor; c1.a = static_cast<unsigned char>(alphaVal[1]);
            float r1 = maxRadius * x[1];

            sf::VertexArray fan(sf::PrimitiveType::TriangleFan, numSegments + 2);
            fan[0] = sf::Vertex(center, c0);
            for (int i = 0; i <= numSegments; ++i)
            {
                float angle = (i * 2.f * 3.14159265f) / numSegments;
                sf::Vector2f pos(
                    center.x + r1 * std::cos(angle),
                    center.y + r1 * std::sin(angle)
                );
                fan[i + 1] = sf::Vertex(pos, c1);
            }
            m_window.draw(fan);
        }

        // 2. Concentric ring bands (from band j to j+1)
        for (int j = 1; j < numBands - 1; ++j)
        {
            sf::Color c_j = baseColor; c_j.a = static_cast<unsigned char>(alphaVal[j]);
            sf::Color c_next = baseColor; c_next.a = static_cast<unsigned char>(alphaVal[j+1]);
            float r_j = maxRadius * x[j];
            float r_next = maxRadius * x[j+1];

            sf::VertexArray strip(sf::PrimitiveType::TriangleStrip, (numSegments + 1) * 2);
            for (int i = 0; i <= numSegments; ++i)
            {
                float angle = (i * 2.f * 3.14159265f) / numSegments;
                float cosA = std::cos(angle);
                float sinA = std::sin(angle);

                sf::Vector2f pos_in(center.x + r_j * cosA, center.y + r_j * sinA);
                sf::Vector2f pos_out(center.x + r_next * cosA, center.y + r_next * sinA);

                strip[2 * i] = sf::Vertex(pos_in, c_j);
                strip[2 * i + 1] = sf::Vertex(pos_out, c_next);
            }
            m_window.draw(strip);
        }
    }

    // Draw Segment Fields (Capacitor Plates) with smooth concentric-band volumetric capsule glow
    for (const auto& field : m_sim.getConfig().segmentFields)
    {
        sf::Vector2f a = toSF(field.start);
        sf::Vector2f b = toSF(field.end);
        sf::Vector2f dir = b - a;
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (len < 0.001f) continue;

        sf::Vector2f tangent = dir / len;
        sf::Vector2f normal(-tangent.y, tangent.x);

        // Ambient reach: larger spread but very pale/low opacity (maxAlpha 90.f) for subtle decay
        float maxWidth = std::min(150.f, std::sqrt(std::abs(field.intensity)) * 2.5f);
        if (maxWidth < 5.f) maxWidth = 50.f;

        // Custom colors: attractive amber/yellow or repulsive sky-blue
        sf::Color baseColor = (field.intensity < 0.f) ? sf::Color(255, 180, 70) : sf::Color(80, 190, 255);

        // Smooth bell-curve function: f(x) = (1 - x^2)^2. Samples boundaries to create seamless transition bands
        const int numBands = 8;
        float x[numBands] = { 0.0f, 0.15f, 0.3f, 0.45f, 0.6f, 0.75f, 0.9f, 1.0f };
        float alphaVal[numBands];
        float maxAlpha = 90.f;
        for (int i = 0; i < numBands; ++i)
        {
            float val = 1.f - x[i] * x[i];
            alphaVal[i] = maxAlpha * val * val;
        }

        const int numCapSegments = 24;
        float baseAngle = std::atan2(tangent.y, tangent.x);

        for (int j = 0; j < numBands - 1; ++j)
        {
            sf::Color c_in = baseColor; c_in.a = static_cast<unsigned char>(alphaVal[j]);
            sf::Color c_out = baseColor; c_out.a = static_cast<unsigned char>(alphaVal[j+1]);
            float w_in = maxWidth * x[j];
            float w_out = maxWidth * x[j+1];

            // A. Left-side body strip for this band (zero overlap)
            sf::VertexArray leftStrip(sf::PrimitiveType::TriangleStrip, 4);
            leftStrip[0] = sf::Vertex(a + normal * w_in, c_in);
            leftStrip[1] = sf::Vertex(a + normal * w_out, c_out);
            leftStrip[2] = sf::Vertex(b + normal * w_in, c_in);
            leftStrip[3] = sf::Vertex(b + normal * w_out, c_out);
            m_window.draw(leftStrip);

            // B. Right-side body strip for this band (zero overlap)
            sf::VertexArray rightStrip(sf::PrimitiveType::TriangleStrip, 4);
            rightStrip[0] = sf::Vertex(a - normal * w_in, c_in);
            rightStrip[1] = sf::Vertex(a - normal * w_out, c_out);
            rightStrip[2] = sf::Vertex(b - normal * w_in, c_in);
            rightStrip[3] = sf::Vertex(b - normal * w_out, c_out);
            m_window.draw(rightStrip);

            // C. Cap A semi-circular ring band for this band (zero overlap)
            sf::VertexArray capA(sf::PrimitiveType::TriangleStrip, (numCapSegments + 1) * 2);
            for (int i = 0; i <= numCapSegments; ++i)
            {
                float angle = baseAngle + 3.14159265f * 0.5f + (i * 3.14159265f) / numCapSegments;
                float cosA = std::cos(angle);
                float sinA = std::sin(angle);

                sf::Vector2f pos_in(a.x + w_in * cosA, a.y + w_in * sinA);
                sf::Vector2f pos_out(a.x + w_out * cosA, a.y + w_out * sinA);

                capA[2 * i] = sf::Vertex(pos_in, c_in);
                capA[2 * i + 1] = sf::Vertex(pos_out, c_out);
            }
            m_window.draw(capA);

            // D. Cap B semi-circular ring band for this band (zero overlap)
            sf::VertexArray capB(sf::PrimitiveType::TriangleStrip, (numCapSegments + 1) * 2);
            for (int i = 0; i <= numCapSegments; ++i)
            {
                float angle = baseAngle - 3.14159265f * 0.5f + (i * 3.14159265f) / numCapSegments;
                float cosA = std::cos(angle);
                float sinA = std::sin(angle);

                sf::Vector2f pos_in(b.x + w_in * cosA, b.y + w_in * sinA);
                sf::Vector2f pos_out(b.x + w_out * cosA, b.y + w_out * sinA);

                capB[2 * i] = sf::Vertex(pos_in, c_in);
                capB[2 * i + 1] = sf::Vertex(pos_out, c_out);
            }
            m_window.draw(capB);
        }

        // Draw the center plate line (slightly glowing and thin)
        sf::VertexArray centerLine(sf::PrimitiveType::Lines, 2);
        sf::Color lineCol = baseColor;
        lineCol.a = 150; 
        centerLine[0] = sf::Vertex(a, lineCol);
        centerLine[1] = sf::Vertex(b, lineCol);
        m_window.draw(centerLine);
    }

    // Draw Outer Boundary Walls
    if (!m_sim.getEnvironment().getBoundarySegments().empty())
    {
        for (const auto& seg : m_sim.getEnvironment().getBoundarySegments())
        {
            sf::Vector2f start = toSF(seg.start);
            sf::Vector2f end = toSF(seg.end);
            sf::Vector2f dir = end - start;
            float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            if (length < 0.001f) continue;

            sf::RectangleShape lineShape(sf::Vector2f(length, 4.0f));
            lineShape.setOrigin({0.f, 2.0f});
            lineShape.setPosition(start);
            lineShape.setRotation(sf::radians(std::atan2(dir.y, dir.x)));
            lineShape.setFillColor(sf::Color::White);
            m_window.draw(lineShape);
        }
    }

    // Draw Particles
    for (const auto& p : m_sim.getParticles())
    {
        sf::CircleShape circle(p.getRadius());
        circle.setPosition(toSF(p.getPosition()));
        circle.setOrigin({p.getRadius(), p.getRadius()});
        circle.setFillColor(sf::Color::Cyan); // PARTICLE_COLOR = Cyan
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
    sf::Color minorColor(90, 98, 110, 60); // GRID_MINOR_COLOR = (90, 98, 110, 60)
    sf::Color majorColor(125, 135, 150, 90); // GRID_MAJOR_COLOR = (125, 135, 150, 90)

    for (float x = startX; x <= endX; x += gridSize)
    {
        if (std::abs(x) < 0.001f) continue;
        int idx = static_cast<int>(std::round(x / gridSize));
        sf::Color color = (idx % 5 == 0) ? majorColor : minorColor;
        lines.append(sf::Vertex({x, top}, color));
        lines.append(sf::Vertex({x, bottom}, color));
    }

    for (float y = startY; y <= endY; y += gridSize)
    {
        if (std::abs(y) < 0.001f) continue;
        int idx = static_cast<int>(std::round(y / gridSize));
        sf::Color color = (idx % 5 == 0) ? majorColor : minorColor;
        lines.append(sf::Vertex({left, y}, color));
        lines.append(sf::Vertex({right, y}, color));
    }

    m_window.draw(lines);

    sf::VertexArray axes(sf::PrimitiveType::Lines);

    if (top <= 0.f && bottom >= 0.f)
    {
        axes.append(sf::Vertex({left, 0.f}, sf::Color(220, 150, 150, 150))); // X_AXIS_COLOR = (220, 150, 150, 150)
        axes.append(sf::Vertex({right, 0.f}, sf::Color(220, 150, 150, 150)));
    }

    if (left <= 0.f && right >= 0.f)
    {
        axes.append(sf::Vertex({0.f, top}, sf::Color(150, 190, 225, 150))); // Y_AXIS_COLOR = (150, 190, 225, 150)
        axes.append(sf::Vertex({0.f, bottom}, sf::Color(150, 190, 225, 150)));
    }

    m_window.draw(axes);

    // Draw Origin Marker (ORIGIN_COLOR = (245, 230, 180, 220), radius = 4.0f, halfSize = 10.0f)
    if (left <= 0.f && right >= 0.f && top <= 0.f && bottom >= 0.f)
    {
        sf::CircleShape originMarker(4.0f);
        originMarker.setPosition({0.f, 0.f});
        originMarker.setOrigin({4.0f, 4.0f});
        originMarker.setFillColor(sf::Color(245, 230, 180, 220));
        m_window.draw(originMarker);

        sf::VertexArray crosshair(sf::PrimitiveType::Lines, 4);
        crosshair[0] = sf::Vertex({-10.0f, 0.f}, sf::Color(245, 230, 180, 220));
        crosshair[1] = sf::Vertex({10.0f, 0.f}, sf::Color(245, 230, 180, 220));
        crosshair[2] = sf::Vertex({0.f, -10.0f}, sf::Color(245, 230, 180, 220));
        crosshair[3] = sf::Vertex({0.f, 10.0f}, sf::Color(245, 230, 180, 220));
        m_window.draw(crosshair);
    }
}

void Visualizer::updateGridCursor(sf::Vector2i mousePosition)
{
    sf::Vector2f worldPos = m_window.mapPixelToCoords(mousePosition);
    sf::Vector2f snappedPos = snapToGrid(worldPos);
    m_gridCursorPreview.setPosition(snappedPos);
}
