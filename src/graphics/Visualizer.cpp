#include "graphics/Visualizer.hpp"

#include <cmath>
#include <iostream>
#include <optional>

namespace
{
sf::Vector2f toSF(const Vector2f& v)
{
    return {v.x, v.y};
}
}

Visualizer::Visualizer(Simulation& sim)
    : m_sim(sim)
{
    const auto& config = m_sim.getConfig();

    m_window.create(sf::VideoMode({config.window.width, config.window.height}), config.window.title);
    m_window.setFramerateLimit(60);

    m_gridCursorPreview.setSize({config.gridSize, config.gridSize});
    m_gridCursorPreview.setFillColor(sf::Color(16, 185, 129, 80));

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

    for (const auto& zone : m_sim.getEnvironment().getZones())
    {
        if (zone->getCompartment() && !zone->getCompartment()->polygon.empty())
        {
            sf::ConvexShape shape(zone->getCompartment()->polygon.size());
            for (size_t i = 0; i < zone->getCompartment()->polygon.size(); ++i)
            {
                shape.setPoint(i, toSF(zone->getCompartment()->polygon[i]));
            }

            sf::Color fillColor;
            sf::Color outlineColor;
            if (zone->getZoneType() == Config::ZoneType::Spawn)
            {
                fillColor = sf::Color(150, 210, 40, 90);
                outlineColor = sf::Color(150, 210, 40, 220);
            }
            else if (zone->getZoneType() == Config::ZoneType::Target)
            {
                fillColor = sf::Color(220, 50, 50, 90);
                outlineColor = sf::Color(220, 50, 50, 220);
            }
            else
            {
                fillColor = sf::Color(148, 163, 184, 90);
                outlineColor = sf::Color(148, 163, 184, 220);
            }

            shape.setFillColor(fillColor);
            shape.setOutlineColor(outlineColor);
            shape.setOutlineThickness(1.f);
            m_window.draw(shape);
        }
    }

    if (!m_sim.getEnvironment().getObstacles().empty())
    {
        for (const auto& wall : m_sim.getEnvironment().getObstacles())
        {
            sf::Vector2f start = toSF(wall.start);
            sf::Vector2f end = toSF(wall.end);
            sf::Vector2f dir = end - start;
            float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            if (length < 0.001f)
            {
                continue;
            }

            sf::RectangleShape lineShape(sf::Vector2f(length, 4.0f));
            lineShape.setOrigin({0.f, 2.0f});
            lineShape.setPosition(start);
            lineShape.setRotation(sf::radians(std::atan2(dir.y, dir.x)));
            lineShape.setFillColor(sf::Color::White);
            m_window.draw(lineShape);
        }
    }

    drawGrid();

    for (const auto& field : m_sim.getFields())
    {
        field->render(m_window);
    }

    if (!m_sim.getEnvironment().getBoundarySegments().empty())
    {
        for (const auto& seg : m_sim.getEnvironment().getBoundarySegments())
        {
            sf::Vector2f start = toSF(seg.start);
            sf::Vector2f end = toSF(seg.end);
            sf::Vector2f dir = end - start;
            float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            if (length < 0.001f)
            {
                continue;
            }

            sf::RectangleShape lineShape(sf::Vector2f(length, 4.0f));
            lineShape.setOrigin({0.f, 2.0f});
            lineShape.setPosition(start);
            lineShape.setRotation(sf::radians(std::atan2(dir.y, dir.x)));
            lineShape.setFillColor(sf::Color::White);
            m_window.draw(lineShape);
        }
    }

    for (const auto& p : m_sim.getParticles())
    {
        sf::CircleShape circle(p.getRadius());
        circle.setPosition(toSF(p.getPosition()));
        circle.setOrigin({p.getRadius(), p.getRadius()});
        circle.setFillColor(sf::Color::Cyan);
        m_window.draw(circle);
    }

    m_window.draw(m_gridCursorPreview);

    if (m_fontLoaded)
    {
        sf::View currentView = m_window.getView();
        m_window.setView(m_window.getDefaultView());

        sf::Vector2u winSize = m_window.getSize();
        float bannerWidth = 100.f;
        float bannerHeight = 35.f;
        float padding = 20.f;

        float x = winSize.x - bannerWidth - padding;
        float y = padding;

        sf::RectangleShape banner({bannerWidth, bannerHeight});
        banner.setPosition({x, y});
        banner.setFillColor(sf::Color(17, 24, 39, 210));
        banner.setOutlineColor(sf::Color(16, 185, 129, 220));
        banner.setOutlineThickness(1.5f);

        sf::Text hudText(m_font);
        int reached = m_sim.getReachedCount();
        int total = m_sim.getParticles().size();
        hudText.setString(std::to_string(reached) + "/" + std::to_string(total));
        hudText.setCharacterSize(16);
        hudText.setFillColor(sf::Color::White);

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
    sf::Color minorColor(90, 98, 110, 60);
    sf::Color majorColor(125, 135, 150, 90);

    for (float x = startX; x <= endX; x += gridSize)
    {
        if (std::abs(x) < 0.001f)
        {
            continue;
        }
        int idx = static_cast<int>(std::round(x / gridSize));
        sf::Color color = (idx % 5 == 0) ? majorColor : minorColor;
        lines.append(sf::Vertex({x, top}, color));
        lines.append(sf::Vertex({x, bottom}, color));
    }

    for (float y = startY; y <= endY; y += gridSize)
    {
        if (std::abs(y) < 0.001f)
        {
            continue;
        }
        int idx = static_cast<int>(std::round(y / gridSize));
        sf::Color color = (idx % 5 == 0) ? majorColor : minorColor;
        lines.append(sf::Vertex({left, y}, color));
        lines.append(sf::Vertex({right, y}, color));
    }

    m_window.draw(lines);

    sf::VertexArray axes(sf::PrimitiveType::Lines);

    if (top <= 0.f && bottom >= 0.f)
    {
        axes.append(sf::Vertex({left, 0.f}, sf::Color(220, 150, 150, 150)));
        axes.append(sf::Vertex({right, 0.f}, sf::Color(220, 150, 150, 150)));
    }

    if (left <= 0.f && right >= 0.f)
    {
        axes.append(sf::Vertex({0.f, top}, sf::Color(150, 190, 225, 150)));
        axes.append(sf::Vertex({0.f, bottom}, sf::Color(150, 190, 225, 150)));
    }

    m_window.draw(axes);

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
