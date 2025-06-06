#pragma once

#include "graph.h" 
#include "plane.h"

class EmbeddablePlane : public Plane{
    public:
        using Plane::Plane;
        void initEmbed(const Graph *graph);
        bool stepForceDirected();
        bool stepForceDirectedStable();
        void embedNode(Node node, double x, double y);
        bool isEmbedding();
    private:
        Point2 getClosestPointOnEdge(const Point2 point, const Point2 end1, const Point2 end2);
        void applyRepulse(double temperature, std::vector<Spot>& spots);
        void applyEdgeRepulse(double temperature, std::vector<Spot>& spots);
        void applyAttraction(double temperature, std::vector<Spot>& spots);
        int currentIteration = -1;
        const int iterationsMax = 500;
        const int repulsionEdgeIterationsMax = 500;
        const int repulsionIterationsMax = 500;
        const double minDistanceRepulsion = 2.0;
        const double kEdgeRepulsion = 5.0f;
        const double kRepulsion = 15.0;
        const double kAttraction = 0.150;
        const double idealLength = 5.0;
        const double minimumTemperature = 0.01;
        const Graph* currentGraph;
};