#pragma once

#include "identifiable.h"
#include "2d.h"
#include <unordered_map>
#include <utility>

// Tile units, independent of passage widths. Zero selects the radius-based default.
struct EdgeGeometry {
    double preferredCenterDistance = 0.0;
    double distanceWeight = 1.0;
};

using EdgeGeometryMap = std::unordered_map<
    std::pair<Identifiable, Identifiable>, EdgeGeometry, PairIDHash>;

// A zone's own positional attraction, independent of magnetic susceptibility.
// Coordinates are normalized to the map rectangle; outside targets are inactive.
struct PositionMagnet {
    DoubleVector2 position;
    double force = 1.0;
};
using PositionMagnetMap = std::unordered_map<Identifiable, PositionMagnet, IDHash>;
