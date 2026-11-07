#include "planar_layout.h"

#include <boost/multiprecision/cpp_int.hpp>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <limits>

namespace planar {
namespace {
DoubleVector2 subtract(DoubleVector2 a, DoubleVector2 b) { return {a.x - b.x, a.y - b.y}; }
double dot(DoubleVector2 a, DoubleVector2 b) { return a.x * b.x + a.y * b.y; }

DoubleVector2 closestPoint(DoubleVector2 p, DoubleVector2 a, DoubleVector2 b) {
    const auto ab = subtract(b, a);
    const double squared = dot(ab, ab);
    const double t = squared > 0 ? std::clamp(dot(subtract(p, a), ab) / squared, 0.0, 1.0) : 0.0;
    return {a.x + t * ab.x, a.y + t * ab.y};
}

bool onSegment(DoubleVector2 a, DoubleVector2 b, DoubleVector2 p) {
    return p.x >= std::min(a.x, b.x) && p.x <= std::max(a.x, b.x)
        && p.y >= std::min(a.y, b.y) && p.y <= std::max(a.y, b.y);
}
} // namespace

int orientation(DoubleVector2 a, DoubleVector2 b, DoubleVector2 c) {
    const double l = (a.x - c.x) * (b.y - c.y);
    const double r = (a.y - c.y) * (b.x - c.x);
    const double det = l - r, magnitude = std::abs(l) + std::abs(r);
    const double bound = 16.0 * std::numeric_limits<double>::epsilon() * magnitude;
    if (std::isfinite(magnitude) && magnitude > 1024 * std::numeric_limits<double>::min()
        && std::abs(det) > bound) return det > 0 ? 1 : -1;

    // Doubles are dyadic rationals. Align all six mantissas to a common power
    // of two and evaluate the determinant exactly, including subnormal inputs.
    using boost::multiprecision::cpp_int;
    const std::array<double, 6> values = {a.x, a.y, b.x, b.y, c.x, c.y};
    std::array<cpp_int, 6> integers;
    std::array<int, 6> exponents;
    int minimum = 0;
    for (size_t i = 0; i < values.size(); ++i) {
        if (!std::isfinite(values[i])) throw std::invalid_argument("Non-finite embedding coordinate");
        const auto bits = std::bit_cast<std::uint64_t>(values[i]);
        const int exponent = static_cast<int>((bits >> 52) & 0x7ff);
        const std::uint64_t mantissa = (bits & ((std::uint64_t{1} << 52) - 1))
            | (exponent ? std::uint64_t{1} << 52 : 0);
        integers[i] = mantissa;
        if (bits >> 63) integers[i] = -integers[i];
        exponents[i] = exponent ? exponent - 1023 - 52 : -1074;
        if (mantissa) minimum = std::min(minimum, exponents[i]);
    }
    for (size_t i = 0; i < values.size(); ++i)
        if (integers[i] != 0) integers[i] <<= exponents[i] - minimum;
    const cpp_int exact = (integers[0] - integers[4]) * (integers[3] - integers[5])
        - (integers[1] - integers[5]) * (integers[2] - integers[4]);
    return exact > 0 ? 1 : exact < 0 ? -1 : 0;
}

bool segmentsIntersect(DoubleVector2 a, DoubleVector2 b, DoubleVector2 c, DoubleVector2 d,
    bool sharedVertex) {
    const int abc = orientation(a, b, c), abd = orientation(a, b, d);
    const int cda = orientation(c, d, a), cdb = orientation(c, d, b);
    if (sharedVertex) {
        return abc == 0 && abd == 0 && cda == 0 && cdb == 0
            && (std::max(std::min(a.x, b.x), std::min(c.x, d.x)) < std::min(std::max(a.x, b.x), std::max(c.x, d.x))
                || std::max(std::min(a.y, b.y), std::min(c.y, d.y)) < std::min(std::max(a.y, b.y), std::max(c.y, d.y)));
    }
    return (abc * abd < 0 && cda * cdb < 0)
        || (abc == 0 && onSegment(a, b, c)) || (abd == 0 && onSegment(a, b, d))
        || (cda == 0 && onSegment(c, d, a)) || (cdb == 0 && onSegment(c, d, b));
}

bool validDrawing(const std::vector<DoubleVector2>& points, const std::vector<Edge>& edges) {
    for (auto p : points) if (!std::isfinite(p.x) || !std::isfinite(p.y)) return false;
    for (auto [a, b] : edges) if (a >= points.size() || b >= points.size() || a == b) return false;
    for (size_t i = 0; i < points.size(); ++i) {
        for (size_t j = 0; j < i; ++j)
            if (points[i].x == points[j].x && points[i].y == points[j].y) return false;
        for (auto [a, b] : edges) {
            if (i != a && i != b && orientation(points[a], points[b], points[i]) == 0
                && onSegment(points[a], points[b], points[i])) return false;
        }
    }
    for (size_t i = 0; i < edges.size(); ++i) {
        const auto [a, b] = edges[i];
        for (size_t j = 0; j < i; ++j) {
            const auto [c, d] = edges[j];
            if (segmentsIntersect(points[a], points[b], points[c], points[d],
                a == c || a == d || b == c || b == d)) return false;
        }
    }
    return true;
}

bool validRasterDrawing(const std::vector<DoubleVector2>& points, const std::vector<Edge>& edges,
    double left, double top, double width, double height) {
    std::vector<DoubleVector2> raster;
    for (const auto& p : points) {
        if (p.x < left || p.y < top || p.x >= left + width || p.y >= top + height) return false;
        raster.emplace_back(std::floor(p.x - left), std::floor(p.y - top));
    }
    return validDrawing(raster, edges);
}

void limitDisplacements(const std::vector<DoubleVector2>& points, const std::vector<Edge>& edges,
    std::vector<DoubleVector2>& movement) {
    if (movement.size() != points.size()) throw std::invalid_argument("Wrong number of node displacements");
    std::vector<double> fraction(points.size(), 1.0);
    const auto constrain = [&](size_t node, DoubleVector2 normal, double allowance) {
        const double progress = dot(normal, movement[node]);
        if (progress > 0) fraction[node] = std::min(fraction[node], std::max(0.0, allowance / progress));
    };
    for (size_t v = 0; v < points.size(); ++v) {
        for (auto [a, b] : edges) {
            if (v == a || v == b) continue;
            const auto q = closestPoint(points[v], points[a], points[b]);
            const auto difference = subtract(points[v], q);
            const double distance = std::hypot(difference.x, difference.y);
            if (distance == 0) throw std::logic_error("Cannot refine a node lying on another edge");
            const DoubleVector2 n = {difference.x / distance, difference.y / distance};
            // Both sides stay strictly separated throughout the entire step.
            constrain(v, {-n.x, -n.y}, 0.45 * distance);
            constrain(a, n, 0.45 * distance - dot(n, subtract(points[a], q)));
            constrain(b, n, 0.45 * distance - dot(n, subtract(points[b], q)));
        }
        for (size_t u = 0; u < v; ++u) {
            const auto difference = subtract(points[v], points[u]);
            const double distance = std::hypot(difference.x, difference.y);
            if (distance == 0) throw std::logic_error("Cannot refine coincident nodes");
            const DoubleVector2 n = {difference.x / distance, difference.y / distance};
            constrain(v, {-n.x, -n.y}, 0.45 * distance);
            constrain(u, n, 0.45 * distance);
        }
    }
    for (size_t v = 0; v < points.size(); ++v) {
        movement[v].x *= fraction[v];
        movement[v].y *= fraction[v];
    }
}
} // namespace planar
