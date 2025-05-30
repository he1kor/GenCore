#include "embedding.h"
#include <random>
#include <iostream>

void EmbeddablePlane::embedGraph(const Graph &graph){
    const int iterations = 500;  // More iterations for better results
    const double kRepulsion = 100.0;  // Increased repulsion strength
    const double kAttraction = 0.5;    // Reduced attraction strength
    const double idealLength = 50.0;   // Target edge length

    std::cout << "embedding...\n";
    int n = graph.size();

    std::vector<Spot> points(n);
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<double> distWidth(0, getWidth());
    std::uniform_real_distribution<double> distHeight(0, getHeight());

    // 1. Random initial placement with minimum distance
    for (int i = 0; i < n; ++i) {
        bool valid = false;
        while (!valid) {
            points[i] = Spot(distWidth(rng), distHeight(rng), i);
            valid = true;
            // Ensure no two points are too close initially
            for (int j = 0; j < i; ++j) {
                double dx = points[j].getX() - points[i].getX();
                double dy = points[j].getY() - points[i].getY();
                double dist = std::sqrt(dx*dx + dy*dy);
                if (dist < idealLength/2) {
                    valid = false;
                    break;
                }
            }
        }
    }

    // 2. Force-directed iteration
    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<Spot> new_points = points;
        double temperature = (1.0 - (double)iter/iterations) * 0.5 + 0.1; // Cooling

        // Repulsive forces (all nodes repel)
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (i == j) continue;
                double dx = points[j].getX() - points[i].getX();
                double dy = points[j].getY() - points[i].getY();
                double dist = std::sqrt(dx*dx + dy*dy) + 0.01;
                
                // Stronger repulsion that dominates at short distances
                double force = kRepulsion * temperature / (dist * dist);
                new_points[i].changeX(-force * dx/dist);
                new_points[i].changeY(-force * dy/dist);
            }
        }

        // Attractive forces (only adjacent nodes)
        for (int i = 0; i < n; ++i) {
            for (int neighbor : graph.getNode(i).getNeighbours()) {
                if (neighbor <= i) continue; // Avoid double-counting edges
                
                double dx = points[neighbor].getX() - points[i].getX();
                double dy = points[neighbor].getY() - points[i].getY();
                double dist = std::sqrt(dx*dx + dy*dy);
                
                // Spring force proportional to displacement from ideal length
                double force = kAttraction * temperature * (dist - idealLength)/dist;
                new_points[i].changeX(force * dx);
                new_points[i].changeY(force * dy);
                new_points[neighbor].changeX(-force * dx);
                new_points[neighbor].changeY(-force * dy);
            }
        }

        // Boundary constraints
        for (int i = 0; i < n; ++i) {
            new_points[i].setX(std::max(0.0, std::min(getWidth(), new_points[i].getX())));
            new_points[i].setY(std::max(0.0, std::min(getHeight(), new_points[i].getY())));
        }

        points = new_points;
    }

    setSpots(points);
    for (auto spot : getSpots()) {
        std::cout << spot.getX() << '\t' << spot.getY() << '\n';
    }
}

void EmbeddablePlane::embedNode(Node node, double x, double y){
    addSpot(Spot(x, y, node.getID()));
}