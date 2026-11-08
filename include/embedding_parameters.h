#pragma once

#include "identifiable.h"
#include "2d.h"
#include <unordered_map>
#include <utility>
#include <stdexcept>
#include <string_view>

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

// Incidence with the real graph's unbounded face, independent of coordinates.
enum class BoundaryPlacement { Any, Outer, Inner };
using NodeBoundaryMap = std::unordered_map<Identifiable, BoundaryPlacement, IDHash>;

inline const char* boundaryPlacementName(BoundaryPlacement placement) {
    switch(placement) {
    case BoundaryPlacement::Any:return "any";
    case BoundaryPlacement::Outer:return "outer";
    case BoundaryPlacement::Inner:return "inner";
    }
    throw std::invalid_argument("Unknown node boundary placement");
}
inline BoundaryPlacement boundaryPlacementFromName(std::string_view name) {
    if(name=="any")return BoundaryPlacement::Any;
    if(name=="outer")return BoundaryPlacement::Outer;
    if(name=="inner")return BoundaryPlacement::Inner;
    throw std::invalid_argument("Boundary placement must be any, outer or inner");
}
