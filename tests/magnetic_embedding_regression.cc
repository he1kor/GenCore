#include "embedding.h"
#include "templates.h"
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

static void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

static void close(double a, double b, const char* message) {
    require(std::abs(a - b) < 1e-9, message);
}

static EmbeddablePlane<BasicNode> controlledGrid(double susceptibility, bool magnets = true) {
    const auto graph = templates::makeGrid3x3(susceptibility);
    EmbeddablePlane<BasicNode> plane(128.0, 128.0);
    plane.setSize(128.0, 80.0);
    RandomGenerator::instance().setSeed(12419);
    plane.initEmbed(graph);
    std::vector<Spot<BasicNode>> spots;
    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 3; ++x) {
            const int id = y * 3 + x;
            spots.emplace_back(32.0 + x * 32.0, 20.0 + y * 20.0, graph->getValue(id));
        }
    }
    plane.setSpots(spots);
    if (magnets) plane.applyMagnetGrid(cornerMagnets);
    return plane;
}

static double radius(const EmbeddablePlane<BasicNode>& plane) {
    double sum = 0.0;
    for (auto id : plane.getIDs()) {
        const auto point = plane.getSpot(id).getCoords();
        require(std::isfinite(point.x) && std::isfinite(point.y), "Non-finite magnetic position");
        require(point.x >= 0.0 && point.x <= 128.0 && point.y >= 0.0 && point.y <= 80.0, "Node escaped the plane");
        sum += std::hypot(point.x - 64.0, point.y - 40.0);
    }
    return sum / plane.getIDs().size();
}

static void bothAxes(double susceptibility) {
    auto magnetic = controlledGrid(susceptibility);
    auto neutral = controlledGrid(0.0);
    for (int step = 0; step < 20; ++step) {
        require(magnetic.stepForceDirected() && neutral.stepForceDirected(), "Embedding ended early");
        for (auto id : magnetic.getIDs()) {
            close(magnetic.getSpot(id).getX(), neutral.getSpot(id).getX(), "Magnet acted before its activation step");
            close(magnetic.getSpot(id).getY(), neutral.getSpot(id).getY(), "Magnet acted before its activation step");
        }
    }
    const auto before = magnetic.getSpot(1).getCoords(); // Center column: a vertical force only.
    close(before.x, 64.0, "Vertical test node is not on the magnet's X axis");
    magnetic.stepForceDirected();
    neutral.stepForceDirected();
    Magnet center(64.0, 40.0, -1.0);
    const auto expected = center.calculateForce(before, BasicNode(1, susceptibility));
    close(magnetic.getSpot(1).getX(), neutral.getSpot(1).getX(), "Vertical magnetic force incorrectly changed X");
    close(magnetic.getSpot(1).getY() - neutral.getSpot(1).getY(), expected.y, "Vertical magnetic force did not change Y correctly");
    require(susceptibility > 0.0 ? expected.y > 0.0 : expected.y < 0.0, "Unexpected attraction/repulsion direction");

    auto diagonal = controlledGrid(susceptibility);
    auto diagonalNeutral = controlledGrid(0.0);
    for (int step = 0; step < 20; ++step) {
        diagonal.stepForceDirected();
        diagonalNeutral.stepForceDirected();
    }
    const auto diagonalBefore = diagonal.getSpot(0).getCoords();
    const auto diagonalForce = center.calculateForce(diagonalBefore, BasicNode(0, susceptibility));
    diagonal.stepForceDirected();
    diagonalNeutral.stepForceDirected();
    close(diagonal.getSpot(0).getX() - diagonalNeutral.getSpot(0).getX(), diagonalForce.x, "Diagonal force has an incorrect X component");
    close(diagonal.getSpot(0).getY() - diagonalNeutral.getSpot(0).getY(), diagonalForce.y, "Diagonal force has an incorrect Y component");
}

static double trajectory(const char* name, double susceptibility, std::ostream* csv) {
    auto plane = controlledGrid(susceptibility);
    int step = 0;
    auto save = [&] {
        if (!csv) return;
        for (auto id : plane.getIDs()) {
            const auto point = plane.getSpot(id).getCoords();
            *csv << name << ',' << step << ',' << id.getID() << ',' << point.x << ',' << point.y << '\n';
        }
    };
    save();
    while (plane.stepForceDirected()) {
        require(++step <= 120, "Embedding did not terminate");
        radius(plane);
        save();
    }
    require(step == 120, "Embedding ended at an unexpected step");
    return radius(plane);
}

int main(int argc, char** argv) {
    try {
        auto resized = controlledGrid(0.0);
        const auto previous = resized.getSpot(0).getCoords();
        resized.setSize(100.0, 60.0);
        close(resized.getRightX(), 100.0, "Resized width is incorrect");
        close(resized.getBottomY(), 60.0, "Resized height is incorrect");
        close(resized.getSpot(0).getX(), previous.x, "Resizing moved an existing spot");
        close(resized.getSpot(0).getY(), previous.y, "Resizing moved an existing spot");
        for (double invalid : {0.0, -1.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()}) {
            for (bool width : {false, true}) {
                bool rejected = false;
                try { resized.setSize(width ? invalid : 100.0, width ? 60.0 : invalid); }
                catch (const std::invalid_argument&) { rejected = true; }
                require(rejected, "Invalid plane dimensions were accepted");
                close(resized.getWidth(), 100.0, "Failed resize changed width");
                close(resized.getHeight(), 60.0, "Failed resize changed height");
            }
        }
        require(templates::grid3x3->size() == 9 && templates::grid3x3->getSymEdges().size() == 12,
            "Adapted grid has the wrong topology");
        require(templates::grid3x3->getNeighbours(4).size() == 4 && templates::grid3x3->getNeighbours(0).size() == 2,
            "Grid adjacency is incorrect");
        bothAxes(0.02);
        bothAxes(-0.02);
        auto zero = controlledGrid(0.0);
        auto noMagnets = controlledGrid(0.0, false);
        while (zero.stepForceDirected()) {
            require(noMagnets.stepForceDirected(), "Controls ended at different steps");
            for (auto id : zero.getIDs()) {
                close(zero.getSpot(id).getX(), noMagnets.getSpot(id).getX(), "Zero susceptibility changed X");
                close(zero.getSpot(id).getY(), noMagnets.getSpot(id).getY(), "Zero susceptibility changed Y");
            }
        }
        std::ofstream csv;
        if (argc > 1) {
            csv.open(argv[1]);
            require(bool(csv), "Cannot open trajectory output");
            csv << std::setprecision(17) << "scenario,step,node,x,y\n";
        }
        const double neutral = trajectory("neutral", 0.0, csv ? &csv : nullptr);
        const double attraction = trajectory("attraction", 0.02, csv ? &csv : nullptr);
        const double repulsion = trajectory("repulsion", -0.02, csv ? &csv : nullptr);
        require(attraction < neutral && neutral < repulsion, "Magnet did not contract/expand the grid as expected");
        close(trajectory("attraction", 0.02, nullptr), attraction, "Magnetic embedding is not reproducible");
        std::cout << "Both axes, activation, polarity, zero susceptibility, bounds and repeatability passed\n"
            << "Final mean radius: attraction=" << attraction << ", neutral=" << neutral << ", repulsion=" << repulsion << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
