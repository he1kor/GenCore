#include "embedding.h"
#include "nodes.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using ZoneGraph = Graph<RadialNode>;

static void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

static std::shared_ptr<const ZoneGraph> graph(size_t n, const std::vector<planar::Edge>& edges,
    const std::vector<double>& radii = {}) {
    std::vector<std::pair<RadialNode, std::vector<Identifiable>>> nodes;
    for (size_t i = 0; i < n; ++i) nodes.push_back({RadialNode(7 + static_cast<int>(i) * 11,
        radii.empty() ? 1.0 : radii[i], 0.0), {}});
    for (auto [a, b] : edges) {
        nodes[a].second.push_back(nodes[b].first);
        nodes[b].second.push_back(nodes[a].first);
    }
    return std::make_shared<const ZoneGraph>(nodes);
}

static auto grid(int side) {
    std::vector<planar::Edge> edges;
    for (int y = 0; y < side; ++y) for (int x = 0; x < side; ++x) {
        if (x + 1 < side) edges.emplace_back(y * side + x, y * side + x + 1);
        if (y + 1 < side) edges.emplace_back(y * side + x, (y + 1) * side + x);
    }
    return graph(side * side, edges);
}

static std::vector<DoubleVector2> positions(const EmbeddablePlane<RadialNode>& plane) {
    std::vector<DoubleVector2> result;
    for (auto id : plane.getIDs()) result.push_back(plane.getSpot(id).getCoords());
    return result;
}

static auto draw(std::shared_ptr<const ZoneGraph> input, std::uint64_t seed, int width = 64, int height = 64,
    const EdgeGeometryMap& preferences = {}, std::shared_ptr<const planar::Topology> topology = nullptr) {
    if (!topology) topology = planar::makeTopology(*input);
    EmbeddablePlane<RadialNode> plane(width, height);
    plane.setLeftX(3.0);
    plane.setUpperY(5.0);
    plane.initEmbed(input, preferences, seed, topology);
    require(plane.findEdgeIntersections().empty(), "Initializer produced edge intersections");
    int steps = 0;
    while (plane.stepPlanar()) {
        require(++steps <= 160, "Planar refinement did not terminate");
        require(plane.findEdgeIntersections().empty(), "Refinement introduced edge intersections");
    }
    require(!plane.isEmbedding(), "Completed embedding is still running");
    const auto points = positions(plane);
    require(planar::validDrawing(points, topology->edges), "Invalid continuous drawing");
    require(planar::validRasterDrawing(points, topology->edges, 3, 5, width, height), "Invalid rasterized drawing");
    if (!points.empty()) {
        double minX = points[0].x, maxX = minX, minY = points[0].y, maxY = minY;
        for (auto p : points) {
            minX = std::min(minX, p.x); maxX = std::max(maxX, p.x);
            minY = std::min(minY, p.y); maxY = std::max(maxY, p.y);
        }
        require(std::abs((minX + maxX) / 2 - (3 + width / 2.0)) < 1e-9, "Drawing is not horizontally centered");
        require(std::abs((minY + maxY) / 2 - (5 + height / 2.0)) < 1e-9, "Drawing is not vertically centered");
    }
    return points;
}

static double distance(const std::vector<DoubleVector2>& points, size_t a, size_t b) {
    return std::hypot(points[a].x - points[b].x, points[a].y - points[b].y);
}

int main() {
    try {
        for (bool bipartite : {false, true}) {
            std::vector<planar::Edge> edges;
            const size_t n = bipartite ? 6 : 5;
            for (size_t a = 0; a < n; ++a) for (size_t b = a + 1; b < n; ++b)
                if (!bipartite || (a < 3 && b >= 3)) edges.emplace_back(a, b);
            bool rejected = false;
            try { planar::makeTopology(*graph(n, edges)); }
            catch (const planar::NonPlanarGraph& error) {
                rejected = std::string(error.what()).find("obstruction") != std::string::npos;
            }
            require(rejected, "Nonplanar graph was accepted");
            // Subdivision preserves nonplanarity and evades simple density checks.
            const auto edge = edges.back(); edges.pop_back();
            edges.emplace_back(edge.first, n); edges.emplace_back(n, edge.second);
            rejected = false;
            try { planar::makeTopology(*graph(n + 1, edges)); }
            catch (const planar::NonPlanarGraph&) { rejected = true; }
            require(rejected, "Subdivided nonplanar graph was accepted");
        }
        draw(graph(0, {}), 0);
        draw(graph(1, {}), 0);
        draw(graph(2, {{0, 1}}), 0);
        draw(graph(3, {}), 1);
        draw(graph(8, {{0,1},{1,2},{2,0},{3,4},{4,5},{5,3},{6,7}}), 1);
        draw(graph(7, {{0,1},{1,2},{2,0},{2,3},{3,4},{4,5},{5,3},{3,6}}), 1);
        std::vector<planar::Edge> tree, wheel, nested = {{0,1},{1,2},{0,2}};
        for (size_t v = 1; v < 17; ++v) {
            tree.emplace_back((v - 1) / 2, v);
            wheel.emplace_back(0, v);
            wheel.emplace_back(v, v == 16 ? 1 : v + 1);
        }
        for (size_t v = 3; v < 23; ++v) {
            nested.emplace_back(0, v); nested.emplace_back(1, v); nested.emplace_back(v - 1, v);
        }
        draw(graph(17, tree), 2, 64, 96);
        draw(graph(17, wheel), 2, 128, 64);
        draw(graph(23, nested), 2, 128, 128);
        for (int side : {3, 5}) {
            const auto input = grid(side);
            const auto topology = planar::makeTopology(*input);
            for (int size : {32, 128}) for (int seed = 0; seed < 20; ++seed)
                draw(input, seed, size, size, {}, topology);
            std::cout << side << "x" << side << " grid: 40 seeds/sizes passed\n";
        }

        const auto star = graph(4, {{0,1},{0,2},{0,3}});
        EdgeGeometryMap preferences = {{{7,18}, {5,20}}, {{7,29}, {5,20}}, {{7,40}, {15,20}}};
        const auto stretched = draw(star, 42, 64, 64, preferences);
        require(distance(stretched, 0, 3) > 1.7 * distance(stretched, 0, 1)
            && distance(stretched, 0, 3) > 1.7 * distance(stretched, 0, 2),
            "Preferred long edge did not elongate relative to short edges");
        std::cout << "Requested star 5/5/15, actual " << distance(stretched, 0, 1) << '/'
            << distance(stretched, 0, 2) << '/' << distance(stretched, 0, 3) << '\n';
        draw(graph(3, {{0,1},{1,2},{0,2}}), 4, 64, 64,
            {{{7,18}, {5,1}}, {{18,29}, {5,1}}, {{7,29}, {15,1}}});
        const auto sized = draw(graph(4, {{0,1},{0,2},{0,3}}, {1,3,1,1}), 42);
        require(distance(sized, 0, 1) > 1.2 * distance(sized, 0, 2),
            "Node radii did not influence embedding spacing");

        const std::vector<DoubleVector2> before = {{0,0},{10,0},{10,10},{0,10}};
        const std::vector<planar::Edge> separated = {{0,1},{2,3}};
        std::vector<DoubleVector2> hostile = {{10,20},{-20,30},{30,-30},{-20,-40}};
        planar::limitDisplacements(before, separated, hostile);
        for (double t : {0.25, 0.5, 0.75, 1.0}) {
            auto during = before;
            for (size_t i = 0; i < during.size(); ++i) {
                during[i].x += t * hostile[i].x; during[i].y += t * hostile[i].y;
            }
            require(planar::validDrawing(during, separated), "Movement tunneled through an edge");
        }

        const auto a = draw(grid(3), 12419), b = draw(grid(3), 12419), c = draw(grid(3), 42);
        for (size_t i = 0; i < a.size(); ++i)
            require(a[i].x == b[i].x && a[i].y == b[i].y, "Same seed produced a different drawing");
        bool differentLengths = false;
        for (size_t i = 0; i < a.size(); ++i) for (size_t j = 0; j < i; ++j)
            differentLengths |= std::abs(distance(a, i, j) - distance(c, i, j)) > 0.1;
        require(differentLengths, "Seed variation is only translation/rotation");
        RandomGenerator::instance().setSeed(77);
        const auto expected = RandomGenerator::instance().uint64Range(0, 1000000);
        RandomGenerator::instance().setSeed(77);
        draw(grid(3), 4);
        require(RandomGenerator::instance().uint64Range(0, 1000000) == expected,
            "Embedding consumed the growth/resource random stream");

        // Near-zero and overflow-scale determinants must retain exact signs.
        const double tiny = std::numeric_limits<double>::denorm_min();
        require(planar::orientation({0,0}, {tiny,0}, {0,tiny}) == 1, "Subnormal orientation is incorrect");
        require(planar::orientation({1e300,0}, {0,1e300}, {0,0}) == 1, "Large orientation is incorrect");
        // New layouts exposed the old 32-bit sum in passage-width sampling.
        for (int i = 0; i < 100; ++i) {
            require(RandomGenerator::instance().takenIndex({std::uint64_t{1} << 33, 0}) == 0,
                "Large passage weights selected an unavailable option");
            require(RandomGenerator::instance().takenIndex({std::numeric_limits<uint64_t>::max(), 0,
                std::numeric_limits<uint64_t>::max(), 0}) % 2 == 0,
                "Overflowing weight totals selected a zero weight");
        }
        std::cout << "Planarity, degenerate graph families, preferences, centering, raster safety and randomness passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
