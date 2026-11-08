#pragma once

#include "planar_layout.h"
#include <functional>

namespace planar {

class NoReflection : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};
class ConflictingSymmetryMagnets : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};
class ConflictingSymmetrySettings : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};

struct Reflection {
    // Indices into Topology::ids. Fixed vertices lie on the reflection axis.
    std::vector<size_t> partner;
    // Certified planar drawing, with the canonical axis x = 0.
    std::vector<DoubleVector2> drawing;
    DoubleVector2 normal{1, 0};
    bool constrainedAxis = false;
    bool gridAligned = false;
};

enum class TileAxis { Vertical, Horizontal, MainDiagonal, AntiDiagonal };
// Only square grids admit diagonal cell-to-cell reflections.
std::optional<TileAxis> tileAxisForNormal(DoubleVector2 normal,double width,double height);
DoubleVector2 tileAxisNormal(TileAxis axis);
IntVector2 reflectedTile(IntVector2 point,TileAxis axis,int width,int height);
bool canonicalTile(IntVector2 point,IntVector2 reflected);

// Searches the real graph, never the generator's temporary scaffold. A returned
// pairing has an independently checked straight-line planar realization.
// Outside magnets are inactive. Width/height determine target distances.
std::shared_ptr<const Reflection> findReflection(const Topology& topology,
    const PositionMagnetMap& magnets = {}, double width = 1, double height = 1,
    bool gridAligned = false, const std::function<bool(const Reflection&)>& accept = {});

} // namespace planar
