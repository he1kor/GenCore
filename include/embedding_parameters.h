#pragma once

#include "identifiable.h"
#include <unordered_map>
#include <utility>

// Tile units, independent of passage widths. Zero selects the radius-based default.
struct EdgeGeometry {
    double preferredCenterDistance = 0.0;
    double distanceWeight = 1.0;
};

using EdgeGeometryMap = std::unordered_map<
    std::pair<Identifiable, Identifiable>, EdgeGeometry, PairIDHash>;
