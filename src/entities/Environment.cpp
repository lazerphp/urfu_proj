#include "entities/Environment.hpp"
#include <iostream>

Environment::Environment(const Config& config)
    : m_obstacles(config.obstacles)
{
    buildBoundarySegments(config);
}

void Environment::buildBoundarySegments(const Config& config)
{
    m_boundarySegments.clear();
    m_zones.clear();
    m_targetZonePtr = nullptr;

    for (const auto& zoneConf : config.zones)
    {
        const Config::Compartment* matchedComp = nullptr;
        for (const auto& comp : config.compartments)
        {
            if (comp.name == zoneConf.compartment)
            {
                matchedComp = &comp;
                break;
            }
        }
        if (matchedComp)
        {
            if (zoneConf.type == "target")
            {
                auto targetZone = std::make_unique<TargetZone>(matchedComp);
                m_targetZonePtr = targetZone.get();
                m_zones.push_back(std::move(targetZone));
            }
            else if (zoneConf.type == "spawn")
            {
                m_zones.push_back(std::make_unique<SpawnZone>(matchedComp));
            }
            else
            {
                m_zones.push_back(std::make_unique<Zone>(zoneConf.type, matchedComp));
            }
        }
    }

    struct SegmentInfo {
        Vector2f start;
        Vector2f end;
        bool isShared = false;
    };
    std::vector<SegmentInfo> allSegments;

    for (const auto& comp : config.compartments)
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

    auto approxEqual = [](Vector2f p1, Vector2f p2) {
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

    for (const auto& seg : allSegments)
    {
        if (!seg.isShared)
        {
            m_boundarySegments.push_back({seg.start, seg.end});
        }
    }
}
