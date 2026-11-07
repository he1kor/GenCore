#include "planar_layout.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <random>

namespace planar {
namespace {
DoubleVector2 difference(DoubleVector2 a, DoubleVector2 b) { return {a.x - b.x, a.y - b.y}; }
double length(DoubleVector2 p) { return std::hypot(p.x, p.y); }

std::vector<DoubleVector2> harmonic(const Topology& topology, double width, double height,
    std::mt19937_64& random) {
    const size_t size = topology.scaffoldSize;
    std::vector<DoubleVector2> positions(size);
    std::vector<bool> fixed(size, false);
    std::uniform_real_distribution<double> noise(-0.25, 0.25);
    std::uniform_real_distribution<double> angle(0, 2 * std::numbers::pi);
    const double phase = angle(random);
    for (size_t i = 0; i < topology.boundary.size(); ++i) {
        const auto v = topology.boundary[i];
        fixed[v] = true;
        const double theta = phase + (i + noise(random) * 0.2)
            * 2 * std::numbers::pi / topology.boundary.size();
        positions[v] = {width / 2 + width * 0.44 * std::cos(theta),
            height / 2 + height * 0.44 * std::sin(theta)};
    }
    std::vector<size_t> free;
    std::vector<size_t> index(size, size);
    for (size_t v = 0; v < size; ++v) {
        if (!fixed[v]) { index[v] = free.size(); free.push_back(v); }
    }
    const size_t n = free.size();
    std::vector<std::vector<double>> matrix(n, std::vector<double>(n + 2, 0));
    for (auto [u, v] : topology.scaffoldEdges) {
        const double weight = std::exp(noise(random)) * (fixed[u] || fixed[v] ? 2.0 : 1.0);
        for (auto [a, b] : {Edge{u, v}, Edge{v, u}}) {
            if (fixed[a]) continue;
            auto& row = matrix[index[a]];
            row[index[a]] += weight;
            if (!fixed[b]) row[index[b]] -= weight;
            else { row[n] += weight * positions[b].x; row[n + 1] += weight * positions[b].y; }
        }
    }
    // Partial pivoting is sufficient for these small positive Dirichlet systems.
    // Every solution is independently checked before it can become a drawing.
    for (size_t k = 0; k < n; ++k) {
        size_t pivot = k;
        for (size_t i = k + 1; i < n; ++i)
            if (std::abs(matrix[i][k]) > std::abs(matrix[pivot][k])) pivot = i;
        if (std::abs(matrix[pivot][k]) < 1e-14) return {};
        std::swap(matrix[k], matrix[pivot]);
        for (size_t i = k + 1; i < n; ++i) {
            const double factor = matrix[i][k] / matrix[k][k];
            for (size_t j = k + 1; j < n + 2; ++j) matrix[i][j] -= factor * matrix[k][j];
        }
    }
    for (size_t i = n; i-- > 0;) {
        double x = matrix[i][n], y = matrix[i][n + 1];
        for (size_t j = i + 1; j < n; ++j) {
            x -= matrix[i][j] * positions[free[j]].x;
            y -= matrix[i][j] * positions[free[j]].y;
        }
        positions[free[i]] = {x / matrix[i][i], y / matrix[i][i]};
    }
    positions.resize(topology.ids.size());
    return positions;
}

double clearance(const std::vector<DoubleVector2>& points, const std::vector<Edge>& edges) {
    double result = std::numeric_limits<double>::infinity();
    for (size_t i = 0; i < points.size(); ++i) {
        for (size_t j = 0; j < i; ++j) result = std::min(result, length(difference(points[i], points[j])));
        for (auto [a, b] : edges) {
            if (i == a || i == b) continue;
            const auto ab = difference(points[b], points[a]), ap = difference(points[i], points[a]);
            const double squared = ab.x * ab.x + ab.y * ab.y;
            const double t = squared > 0 ? std::clamp((ap.x * ab.x + ap.y * ab.y) / squared, 0.0, 1.0) : 0.0;
            result = std::min(result, length({ap.x - t * ab.x, ap.y - t * ab.y}));
        }
    }
    return result;
}
} // namespace

Layout::Layout(std::shared_ptr<const Topology> topology, const std::vector<double>& radii,
    const EdgeGeometryMap& preferences, double left, double top, double width, double height,
    std::uint64_t seed)
    : topology(std::move(topology)), left(left), top(top), width(width), height(height) {
    if (!this->topology || !std::isfinite(left) || !std::isfinite(top)
        || !std::isfinite(width) || !std::isfinite(height) || !std::isfinite(width * height)
        || width < 2 || height < 2)
        throw std::invalid_argument("Planar embedding needs finite bounds at least 2 units wide/high");
    const auto& graph = *this->topology;
    const size_t n = graph.ids.size();
    if (radii.size() != n) throw std::invalid_argument("Embedding radii do not match graph nodes");
    if (n == 0) { iteration = iterations; return; }
    double maxRadius = 0;
    for (double r : radii) {
        if (!std::isfinite(r) || r <= 0) throw std::invalid_argument("Embedding radii must be finite and positive");
        maxRadius = std::max(maxRadius, r);
    }
    double totalSquared = 0;
    for (double r : radii) {
        const double normalized = std::max(0.02, r / maxRadius);
        nodeRadii.push_back(normalized);
        totalSquared += normalized * normalized;
    }
    const double scale = std::sqrt(width * height / (std::numbers::pi * totalSquared));
    for (double& r : nodeRadii) r *= scale;

    double largestWeight = 1.0;
    for (const auto& [edge, preference] : preferences) {
        const auto normalized = PairIDHash::normalize(edge);
        const auto a = std::lower_bound(graph.ids.begin(), graph.ids.end(), normalized.first);
        const auto b = std::lower_bound(graph.ids.begin(), graph.ids.end(), normalized.second);
        if (a == graph.ids.end() || *a != normalized.first || b == graph.ids.end() || *b != normalized.second
            || !std::binary_search(graph.edges.begin(), graph.edges.end(), Edge{a - graph.ids.begin(), b - graph.ids.begin()}))
            throw std::invalid_argument("Geometry preference must reference an existing graph edge");
        if (!std::isfinite(preference.preferredCenterDistance) || preference.preferredCenterDistance < 0
            || !std::isfinite(preference.distanceWeight) || preference.distanceWeight < 0)
            throw std::invalid_argument("Edge geometry must be finite and non-negative");
        largestWeight = std::max(largestWeight, preference.distanceWeight);
    }
    for (auto [u, v] : graph.edges) {
        auto it = preferences.find({graph.ids[u], graph.ids[v]});
        if (it == preferences.end()) it = preferences.find({graph.ids[v], graph.ids[u]});
        const auto preference = it == preferences.end() ? EdgeGeometry{} : it->second;
        const double target = preference.preferredCenterDistance > 0
            ? preference.preferredCenterDistance : 0.9 * (nodeRadii[u] + nodeRadii[v]);
        desiredLengths.push_back(std::clamp(target, 1e-6, 1e4 * std::max(width, height)));
        edgeWeights.push_back(preference.distanceWeight / std::max(1.0, largestWeight / 1e4));
    }

    std::mt19937_64 random(seed);
    if (n < 3) {
        for (size_t i = 0; i < n; ++i) points.emplace_back(left + width / 2 + (double(i) - (n - 1) / 2.0)
            * std::min(width * 0.5, n == 2 && !desiredLengths.empty() ? desiredLengths[0] : width / 3), top + height / 2);
    } else {
        double bestClearance = -1;
        for (int attempt = 0; attempt < 3; ++attempt) {
            auto candidate = harmonic(graph, width, height, random);
            if (candidate.empty()) continue;
            for (auto& p : candidate) { p.x += left; p.y += top; }
            center(candidate);
            if (!validDrawing(candidate, graph.edges)) continue;
            const double separation = clearance(candidate, graph.edges);
            if (separation > bestClearance) { points = candidate; bestClearance = separation; }
            if (validRasterDrawing(candidate, graph.edges, left, top, width, height)
                && (bestRasterPoints.empty() || score(candidate) < bestScore)) {
                bestScore = score(candidate);
                bestRasterPoints = std::move(candidate);
            }
        }
        // Fit the certified integer drawing as a fallback. No rounding is
        // trusted: it must pass the same geometric and raster checks.
        auto fallback = graph.gridDrawing;
        double maxX = 0, maxY = 0;
        for (auto p : fallback) { maxX = std::max(maxX, p.x); maxY = std::max(maxY, p.y); }
        double factor = std::min((width - 2) / std::max(1.0, maxX), (height - 2) / std::max(1.0, maxY));
        if (factor >= 1.0) factor = std::floor(factor); // Preserve the integer grid under tile rounding.
        for (auto& p : fallback) { p.x = left + 1 + p.x * factor; p.y = top + 1 + p.y * factor; }
        center(fallback);
        if (validDrawing(fallback, graph.edges)) {
            if (points.empty()) points = fallback;
            if (bestRasterPoints.empty() && validRasterDrawing(fallback, graph.edges, left, top, width, height)) {
                bestScore = score(fallback);
                bestRasterPoints = fallback;
            }
        }
        if (!bestRasterPoints.empty()) points = bestRasterPoints;
    }
    if (points.size() != n || !validDrawing(points, graph.edges))
        throw std::runtime_error("Could not construct a non-degenerate planar drawing");
    shapeTargets = points;
    std::uniform_real_distribution<double> jitter(-0.07, 0.07);
    for (auto& p : shapeTargets) {
        p.x += jitter(random) * std::sqrt(width * height / n);
        p.y += jitter(random) * std::sqrt(width * height / n);
    }
    if (bestRasterPoints.empty() && validRasterDrawing(points, graph.edges, left, top, width, height)) {
        bestRasterPoints = points;
    }
    if (!bestRasterPoints.empty()) bestScore = score(bestRasterPoints);
}

void Layout::center(std::vector<DoubleVector2>& positions) const {
    if (positions.empty()) return;
    double minX = positions[0].x, maxX = minX, minY = positions[0].y, maxY = minY;
    for (auto p : positions) {
        minX = std::min(minX, p.x); maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y); maxY = std::max(maxY, p.y);
    }
    const double dx = left + width / 2 - (minX + maxX) / 2;
    const double dy = top + height / 2 - (minY + maxY) / 2;
    for (auto& p : positions) { p.x += dx; p.y += dy; }
}

double Layout::score(const std::vector<DoubleVector2>& positions) const {
    double result = 0;
    const double scale = width * height / std::max(size_t{1}, positions.size());
    for (size_t e = 0; e < topology->edges.size(); ++e) {
        auto [u, v] = topology->edges[e];
        const double error = length(difference(positions[u], positions[v])) - desiredLengths[e];
        result += 0.06 * edgeWeights[e] * error * error / scale;
    }
    for (size_t i = 0; i < positions.size(); ++i) {
        for (size_t j = 0; j < i; ++j) {
            const double d = std::max(1e-8, length(difference(positions[i], positions[j])));
            const double overlap = std::max(0.0, 0.55 * (nodeRadii[i] + nodeRadii[j]) - d);
            result += 0.06 / d + 0.04 * overlap * overlap / scale;
        }
        if (shapeTargets.size() == positions.size()) {
            const auto delta = difference(positions[i], shapeTargets[i]);
            result += 0.0075 * (delta.x * delta.x + delta.y * delta.y) / scale;
        }
    }
    return result;
}

bool Layout::step(const std::vector<DoubleVector2>& extraForces) {
    if (!isRunning()) return false;
    std::vector<DoubleVector2> movement(points.size());
    const double areaPerNode = width * height / points.size();
    for (size_t e = 0; e < topology->edges.size(); ++e) {
        const auto [u, v] = topology->edges[e];
        const auto delta = difference(points[v], points[u]);
        const double d = std::max(1e-8, length(delta));
        const double force = 0.12 * edgeWeights[e] * (d - desiredLengths[e]) / d;
        movement[u].x += force * delta.x; movement[u].y += force * delta.y;
        movement[v].x -= force * delta.x; movement[v].y -= force * delta.y;
    }
    for (size_t i = 0; i < points.size(); ++i) {
        for (size_t j = 0; j < i; ++j) {
            const auto delta = difference(points[i], points[j]);
            const double d = std::max(1e-8, length(delta));
            const double overlap = std::max(0.0, 0.55 * (nodeRadii[i] + nodeRadii[j]) - d);
            const double force = 0.06 * areaPerNode / (d * d * d) + 0.08 * overlap / d;
            movement[i].x += force * delta.x; movement[i].y += force * delta.y;
            movement[j].x -= force * delta.x; movement[j].y -= force * delta.y;
        }
    }
    // Accumulate every pair force before limiting any node's total movement.
    for (size_t i = 0; i < points.size(); ++i) {
        movement[i].x += 0.015 * (shapeTargets[i].x - points[i].x);
        movement[i].y += 0.015 * (shapeTargets[i].y - points[i].y);
        if (!extraForces.empty()) {
            if (extraForces.size() != points.size()) throw std::invalid_argument("Wrong number of embedding forces");
            movement[i].x += extraForces[i].x; movement[i].y += extraForces[i].y;
        }
        const double maxStep = std::min(width, height) * 0.02 * (1.0 - 0.8 * iteration / iterations);
        const double magnitude = length(movement[i]);
        if (!std::isfinite(magnitude)) throw std::invalid_argument("Non-finite embedding force");
        if (magnitude > maxStep) { movement[i].x *= maxStep / magnitude; movement[i].y *= maxStep / magnitude; }
        // Restrict the displacement, rather than clamping positions afterwards.
        double fraction = 1;
        if (movement[i].x < 0) fraction = std::min(fraction, (points[i].x - left - 0.5) / -movement[i].x);
        if (movement[i].x > 0) fraction = std::min(fraction, (left + width - 0.5 - points[i].x) / movement[i].x);
        if (movement[i].y < 0) fraction = std::min(fraction, (points[i].y - top - 0.5) / -movement[i].y);
        if (movement[i].y > 0) fraction = std::min(fraction, (top + height - 0.5 - points[i].y) / movement[i].y);
        fraction = std::clamp(fraction, 0.0, 1.0);
        movement[i].x *= fraction; movement[i].y *= fraction;
    }
    limitDisplacements(points, topology->edges, movement);
    auto candidate = points;
    for (size_t i = 0; i < points.size(); ++i) {
        candidate[i].x += movement[i].x; candidate[i].y += movement[i].y;
    }
    center(candidate);
    // Independent validation also guards finite-precision errors in movement bounds.
    if (validDrawing(candidate, topology->edges)) points = std::move(candidate);
    const double quality = score(points);
    const bool external = std::any_of(extraForces.begin(), extraForces.end(),
        [](DoubleVector2 force) { return force.x != 0 || force.y != 0; });
    if (validRasterDrawing(points, topology->edges, left, top, width, height)
        && (bestRasterPoints.empty() || quality < bestScore || external)) {
        bestScore = quality;
        bestRasterPoints = points;
    }
    if (++iteration == iterations) {
        if (bestRasterPoints.empty())
            throw std::runtime_error("Could not find a planar drawing that survives tile rounding at this size; choose a larger map");
        points = bestRasterPoints;
        if (!validDrawing(points, topology->edges)
            || !validRasterDrawing(points, topology->edges, left, top, width, height))
            throw std::logic_error("Invalid final planar embedding");
    }
    return true;
}
} // namespace planar
