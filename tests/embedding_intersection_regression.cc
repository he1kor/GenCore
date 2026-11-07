#include "embedding.h"
#include "nodes.h"

#include <sstream>
#include <stdexcept>

static void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

static EmbeddablePlane<BasicNode> layout(
    const std::vector<DoubleVector2>& points,
    const std::vector<std::pair<int, int>>& edges) {
    std::vector<std::pair<BasicNode, std::vector<Identifiable>>> nodes;
    std::vector<Spot<BasicNode>> spots;
    for (size_t i = 0; i < points.size(); ++i) {
        // Sparse IDs and descending node order exercise edge normalization.
        const int id = 100 - static_cast<int>(i) * 10;
        nodes.push_back({BasicNode(id, 0.0), {}});
        spots.emplace_back(points[i].x, points[i].y, nodes.back().first);
    }
    for (const auto& [a, b] : edges) {
        nodes[a].second.push_back(nodes[b].first);
        nodes[b].second.push_back(nodes[a].first);
    }
    auto graph = std::make_shared<Graph<BasicNode>>(nodes);
    EmbeddablePlane<BasicNode> plane(128.0, 128.0);
    RandomGenerator::instance().setSeed(12419);
    plane.initEmbed(graph);
    plane.setSpots(spots);
    return plane;
}

static void independentEdges(const std::vector<DoubleVector2>& points, bool intersects, const char* message) {
    auto plane = layout(points, {{0, 1}, {2, 3}});
    require(plane.findEdgeIntersections().size() == (intersects ? 1u : 0u), message);
}

// Restore cerr even if a regression assertion throws.
class CaptureWarnings {
public:
    CaptureWarnings() : previous(std::cerr.rdbuf(output.rdbuf())) {}
    ~CaptureWarnings() { std::cerr.rdbuf(previous); }
    std::ostringstream output;
private:
    std::streambuf* previous;
};

static void completionWarning() {
    auto plane = layout({{20, 20}, {100, 100}, {20, 100}, {100, 20}}, {{0, 1}, {2, 3}});
    const auto finalSpots = std::vector<Spot<BasicNode>>{
        plane.getSpot(100), plane.getSpot(90), plane.getSpot(80), plane.getSpot(70)};
    CaptureWarnings warnings;
    for (int i = 0; i < 119; ++i) require(plane.stepForceDirected(), "Embedding ended early");
    require(warnings.output.str().empty(), "Intermediate steps emitted final-layout warnings");
    plane.setSpots(finalSpots);
    require(plane.stepForceDirected() && !plane.isEmbedding(), "Last embedding step did not finish");
    require(plane.findEdgeIntersections().size() == 1, "Completion test lost its crossing");
    const auto warning = warnings.output.str();
    require(warning.find("Warning: embedding has 1 intersecting edge pair(s)") != std::string::npos,
        "Completion did not warn about the final crossing");
    require(warning.find("(70, 80)") != std::string::npos && warning.find("(90, 100)") != std::string::npos,
        "Warning did not identify both offending edges");
    require(!plane.stepForceDirected() && warnings.output.str() == warning,
        "Polling a finished embedding repeated its warning");

    // A new run on the same plane must validate the new graph and positions.
    const auto cleanGraph = std::make_shared<Graph<BasicNode>>(
        std::vector<std::pair<BasicNode, std::vector<Identifiable>>>{
            {BasicNode(10, 0.0), {20}}, {BasicNode(20, 0.0), {10}}});
    plane.initEmbed(cleanGraph);
    warnings.output.str("");
    while (plane.stepForceDirected()) {}
    require(warnings.output.str().empty(), "A clean embedding emitted a warning");
}

int main() {
    try {
        EmbeddablePlane<BasicNode> uninitialized(128.0, 128.0);
        require(uninitialized.findEdgeIntersections().empty(), "Uninitialized plane has intersections");
        require(layout({{20, 20}}, {}).findEdgeIntersections().empty(), "Edgeless graph has intersections");
        require(layout({{20, 20}, {80, 80}}, {{0, 1}}).findEdgeIntersections().empty(),
            "A single undirected edge was counted twice");

        independentEdges({{0, 0}, {10, 10}, {0, 10}, {10, 0}}, true, "Missed diagonal crossing");
        independentEdges({{5, 0}, {5, 10}, {0, 5}, {10, 5}}, true, "Missed axis-aligned crossing");
        independentEdges({{0, 0}, {2, 2}, {3, 0}, {3, 1}}, false, "Intersecting infinite lines are disjoint segments");
        independentEdges({{0, 0}, {10, 0}, {0, 1e-12}, {10, 1e-12}}, false, "Nearby parallel edges intersect");
        independentEdges({{0, 0}, {10, 0}, {5, -1e-12}, {5, 1e-12}}, true, "Missed very shallow crossing");
        independentEdges({{0, 0}, {2, 2}, {3, 3}, {4, 4}}, false, "Disjoint collinear edges intersect");
        independentEdges({{0, 0}, {10, 0}, {5, 0}, {15, 0}}, true, "Missed horizontal overlap");
        independentEdges({{0, 0}, {0, 10}, {0, 5}, {0, 15}}, true, "Missed vertical overlap");
        independentEdges({{10, 10}, {0, 0}, {2, 2}, {8, 8}}, true, "Missed contained reversed diagonal overlap");
        independentEdges({{0, 0}, {10, 10}, {10, 10}, {0, 0}}, true, "Missed coincident edges with different IDs");
        independentEdges({{0, 0}, {5, 0}, {5, 0}, {10, 0}}, true, "Missed collinear endpoint contact");
        independentEdges({{0, 0}, {10, 0}, {5, 0}, {5, 10}}, true, "Missed T-junction");
        independentEdges({{0, 0}, {5, 5}, {5, 5}, {10, 0}}, true, "Coincident positions with distinct IDs are not shared vertices");
        independentEdges({{5, 0}, {5, 0}, {0, 0}, {10, 0}}, true, "Missed collapsed edge on another edge");
        independentEdges({{5, 1}, {5, 1}, {0, 0}, {10, 0}}, false, "Off-edge collapsed segment intersects");
        independentEdges({{5, 1}, {5, 1}, {5, 1}, {5, 1}}, true, "Missed coincident collapsed edges");
        independentEdges({{5, 1}, {5, 1}, {6, 1}, {6, 1}}, false, "Distinct collapsed edges intersect");

        require(layout({{5, 5}, {10, 5}, {5, 10}}, {{0, 1}, {0, 2}}).findEdgeIntersections().empty(),
            "Legal shared graph vertex was flagged");
        require(layout({{5, 5}, {0, 5}, {10, 5}}, {{0, 1}, {0, 2}}).findEdgeIntersections().empty(),
            "Opposite collinear rays were flagged");
        require(layout({{0, 0}, {10, 0}, {5, 0}}, {{0, 1}, {0, 2}}).findEdgeIntersections().size() == 1,
            "Missed overlapping incident horizontal edges");
        require(layout({{0, 0}, {0, 10}, {0, 5}}, {{0, 1}, {0, 2}}).findEdgeIntersections().size() == 1,
            "Missed overlapping incident vertical edges");
        require(layout({{0, 0}, {10, 10}, {5, 5}}, {{0, 1}, {0, 2}}).findEdgeIntersections().size() == 1,
            "Missed overlapping incident diagonal edges");

        const std::vector<DoubleVector2> points = {{0, 0}, {10, 10}, {0, 10}, {10, 0}, {5, 0}, {5, 10}};
        auto multiple = layout(points,
            {{0, 1}, {2, 3}, {4, 5}});
        const auto intersections = multiple.findEdgeIntersections();
        require(intersections.size() == 3, "Did not report every intersecting pair exactly once");
        require(intersections[0].firstEdge == std::make_pair(Identifiable(50), Identifiable(60))
            && intersections[0].secondEdge == std::make_pair(Identifiable(70), Identifiable(80))
            && intersections[1].secondEdge == std::make_pair(Identifiable(90), Identifiable(100))
            && intersections[2].firstEdge == intersections[0].secondEdge,
            "Intersection order is not deterministic");
        std::ostringstream warning;
        require(multiple.warnEdgeIntersections(warning) == 3, "Warning returned the wrong pair count");
        require(warning.str().find("3 intersecting edge pair(s)") != std::string::npos,
            "Warning omitted the total pair count");
        for (size_t i = 0; i < points.size(); ++i) {
            const auto actual = multiple.getSpot(multiple.getIDs()[i]).getCoords();
            require(actual.x == points[i].x && actual.y == points[i].y,
                "Validation moved an embedded point");
        }
        require(multiple.findEdgeIntersections().size() == 3, "Repeated validation changed the result");
        completionWarning();
        std::cout << "Crossings, contacts, overlaps, shared vertices, ordering and completion warnings passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
