#pragma once

#include "2d.h"
#include "embedding_parameters.h"
#include "graph.h"
#include <cstdint>
#include <memory>
#include <stdexcept>

namespace planar {

using Edge = std::pair<size_t, size_t>;

class NonPlanarGraph : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};

// No Boost types escape the implementation. Original IDs remain stable even
// when temporary edges/vertices are added to the drawing graph.
struct Topology {
    std::vector<Identifiable> ids;
    std::vector<Edge> edges;
    std::vector<Edge> scaffoldEdges;
    std::vector<size_t> boundary;
    // Real vertices around the outer face, before adding the private frame.
    // Editors can use this order for a rectangular diagram boundary.
    std::vector<size_t> outerFace;
    size_t scaffoldSize = 0;
    std::vector<DoubleVector2> gridDrawing;
};

std::shared_ptr<const Topology> makeTopology(std::vector<Identifiable> ids,
    const std::vector<std::pair<Identifiable, Identifiable>>& edges);

template<hasID T>
std::shared_ptr<const Topology> makeTopology(const Graph<T>& graph) {
    std::vector<std::pair<Identifiable, Identifiable>> edges;
    for (auto id : graph.getIDs()) {
        for (auto neighbour : graph.getNeighbours(id)) edges.emplace_back(id, neighbour);
    }
    return makeTopology(graph.getIDs(), edges);
}

// Exact determinant sign for finite double inputs, with a filtered fast path.
int orientation(DoubleVector2 a, DoubleVector2 b, DoubleVector2 c);
bool segmentsIntersect(DoubleVector2 a, DoubleVector2 b, DoubleVector2 c, DoubleVector2 d,
    bool sharedVertex = false);
bool validDrawing(const std::vector<DoubleVector2>& positions, const std::vector<Edge>& edges);
bool validRasterDrawing(const std::vector<DoubleVector2>& positions, const std::vector<Edge>& edges,
    double left, double top, double width, double height);

// PrEd-style separating half-planes limit simultaneous displacements. Forces
// choose desirable movement; these constraints determine safe movement.
void limitDisplacements(const std::vector<DoubleVector2>& positions,
    const std::vector<Edge>& edges, std::vector<DoubleVector2>& displacements);

class Layout {
public:
    Layout(std::shared_ptr<const Topology> topology, const std::vector<double>& radii,
        const EdgeGeometryMap& preferences, double left, double top, double width, double height,
        std::uint64_t seed, const PositionMagnetMap& magnets = {});
    bool step(const std::vector<DoubleVector2>& extraForces = {});
    bool isRunning() const { return iteration < iterations; }
    const std::vector<DoubleVector2>& positions() const { return points; }

private:
    std::shared_ptr<const Topology> topology;
    std::vector<DoubleVector2> points;
    std::vector<DoubleVector2> bestRasterPoints;
    std::vector<DoubleVector2> shapeTargets;
    std::vector<double> desiredLengths;
    std::vector<double> edgeWeights;
    std::vector<double> nodeRadii;
    std::vector<double> magnetForces;
    std::vector<DoubleVector2> magnetTargets;
    bool hasPositionMagnets = false;
    double left, top, width, height;
    int iteration = 0;
    static constexpr int iterations = 160;
    double bestScore = 0.0;
    void center(std::vector<DoubleVector2>& positions) const;
    double score(const std::vector<DoubleVector2>& positions) const;
    void initializePositionMagnets();
    void limitMagnetCrowding(std::vector<DoubleVector2>& movement) const;
};

} // namespace planar
