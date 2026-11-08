#pragma once



#include <iostream>
#include <vector>
#include <list>
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <stack>
#include <cassert>
#include <utility>
#include <optional>

#include "math_util.h"
#include "graph.h" 
#include "plane.h"
#include "grid.h"

#include "random_generator.h"
#include "planar_layout.h"
#include "radial.h"

struct IdentifiedMagnet : public Identifiable, public Magnet{
    public:
        IdentifiedMagnet(double force) : Identifiable(id), Magnet(force){id++;};
        IdentifiedMagnet(int id = Identifiable::nullID) : Identifiable(id), Magnet(0){};
        static inline int id = 0;
};

static Grid<IdentifiedMagnet> cornerMagnets = Grid<IdentifiedMagnet>(
    {
        {{}, {}, {}},
        {{}, -1.0, {}},
        {{}, {}, {}}
    }
);

struct EdgeIntersection {
    std::pair<Identifiable, Identifiable> firstEdge;
    std::pair<Identifiable, Identifiable> secondEdge;
};


template<typename T>
class EmbeddablePlane : public Plane<T>{
    public:
        using Plane<T>::Plane;
        // Default: planar construction and topology-preserving refinement.
        void initEmbed(std::shared_ptr<const Graph<T>> graph, const EdgeGeometryMap& preferences = {},
            std::uint64_t seed = RandomGenerator::instance().getSeed(),
            std::shared_ptr<const planar::Topology> topology = nullptr,
            const PositionMagnetMap& positionMagnets = {});
        bool stepPlanar();
        // Legacy unconstrained algorithm, retained for comparisons.
        void initForceDirected(std::shared_ptr<const Graph<T>> graph);
        bool stepForceDirected();
        bool stepForceDirectedStable();
        void embedNode(Node<T> node, double x, double y);
        void embed(std::shared_ptr<const Graph<T>> graph);
        bool isEmbedding();
        void updateTempSpots();
        void setupTempSpots();
        void calculateZoneRadius();
        void applyMagnetGrid(Grid<IdentifiedMagnet> magnetRelativePoses);
        // O(E^2). Reports crossings, overlaps and non-shared endpoint contacts.
        // Edges may meet at their shared graph vertex, but may not overlap there.
        std::vector<EdgeIntersection> findEdgeIntersections() const;
        size_t warnEdgeIntersections(std::ostream& output = std::cerr) const;

    private:
        DoubleVector2 getClosestPointOnEdge(const DoubleVector2 point, const DoubleVector2 end1, const DoubleVector2 end2);
        void applyRepulse(double temperature);
        void applyBorderRepulseToSpot(Identifiable id, double temperature);
        void clampToBorder(Identifiable id);
        void commitSpots();
        void applyEdgeRepulse(double temperature);
        void applyMagnetForces(double temperature);
        void applyAttraction(double temperature);
        void publishPlanarSpots();

        
        int currentIteration = -1;
        double idealLength = 5.0;
        
        std::shared_ptr<const Graph<T>> currentGraph;
        std::unordered_map<Identifiable, Spot<T>, IDHash> temp_spots;
        std::vector<IdentifiedMagnet> magnets;
        std::shared_ptr<const planar::Topology> planarTopology;
        std::optional<planar::Layout> planarLayout;
        int planarIteration = 0;
        
        static constexpr int iterationsMax = 100;
        static constexpr int repulsionEdgeIterationsMax = 100;
        static constexpr int repulsionIterationsMax = 120;
        static constexpr double minDistanceRepulsion = 2.0;
        static constexpr double kEdgeRepulsion = 30.0f;
        static constexpr double kRepulsion = 17.0;
        static constexpr double kBorderRepulsion = 57.0;
        static constexpr double kAttraction = 0.150;
        static constexpr bool squareDistance = true;
        static constexpr double minimumTemperature = 0.01;
        static constexpr double magnetForceIterationMin = 20;
        static constexpr double magnetForceIterationMax = 120;
};


template<typename T>
void EmbeddablePlane<T>::initEmbed(std::shared_ptr<const Graph<T>> graph, const EdgeGeometryMap& preferences,
    std::uint64_t seed, std::shared_ptr<const planar::Topology> topology, const PositionMagnetMap& positionMagnets) {
    if (!graph) throw std::invalid_argument("No embedding graph supplied");
    if (!topology) topology = planar::makeTopology(*graph);
    std::vector<Identifiable> ids = graph->getIDs();
    std::sort(ids.begin(), ids.end());
    if (ids != topology->ids) throw std::invalid_argument("Cached embedding topology does not match graph nodes");
    std::vector<planar::Edge> actualEdges;
    for (size_t u = 0; u < ids.size(); ++u) {
        for (auto neighbour : graph->getNeighbours(ids[u])) {
            const auto it = std::lower_bound(ids.begin(), ids.end(), neighbour);
            if (it == ids.end() || *it != neighbour) throw std::invalid_argument("Missing graph endpoint");
            const size_t v = it - ids.begin();
            actualEdges.emplace_back(std::min(u, v), std::max(u, v));
        }
    }
    std::sort(actualEdges.begin(), actualEdges.end());
    actualEdges.erase(std::unique(actualEdges.begin(), actualEdges.end()), actualEdges.end());
    if (actualEdges != topology->edges) throw std::invalid_argument("Cached embedding topology does not match graph edges");
    std::vector<double> radii;
    for (auto id : topology->ids) {
        if constexpr (std::is_base_of_v<Radial, T>) radii.push_back(graph->getValue(id).getRadius());
        else radii.push_back(1.0);
    }
    planar::Layout layout(topology, radii, preferences, this->getLeftX(), this->getUpperY(),
        this->getWidth(), this->getHeight(), seed, positionMagnets);
    currentGraph = std::move(graph);
    planarTopology = std::move(topology);
    planarLayout = std::move(layout);
    currentIteration = -1;
    planarIteration = 0;
    temp_spots.clear();
    publishPlanarSpots();
}

template<typename T>
void EmbeddablePlane<T>::publishPlanarSpots() {
    std::vector<Spot<T>> spots;
    const auto& positions = planarLayout->positions();
    for (size_t i = 0; i < positions.size(); ++i)
        spots.emplace_back(positions[i].x, positions[i].y, currentGraph->getValue(planarTopology->ids[i]));
    this->setSpots(spots);
}

template<typename T>
bool EmbeddablePlane<T>::stepPlanar() {
    if (!planarLayout || !planarLayout->isRunning()) return false;
    std::vector<DoubleVector2> forces(planarTopology->ids.size());
    if constexpr (std::is_base_of_v<Magnetic, T>) {
        if (planarIteration >= magnetForceIterationMin) {
            for (size_t i = 0; i < forces.size(); ++i) {
                const auto id = planarTopology->ids[i];
                for (const auto& magnet : magnets) {
                    const auto force = magnet.calculateForce(this->getSpot(id).getCoords(), this->getValue(id));
                    forces[i].x += force.x; forces[i].y += force.y;
                }
            }
        }
    }
    if (!planarLayout->step(forces)) return false;
    ++planarIteration;
    publishPlanarSpots();
    return true;
}

template<typename T> void EmbeddablePlane<T>::initForceDirected(std::shared_ptr<const Graph<T>> graph){
    planarLayout.reset();
    planarTopology.reset();
    temp_spots.clear();
    this->currentGraph = graph;
    calculateZoneRadius();
    this->clear();
    
    
    int attempts = 0;
    for (int i = 0; i < graph->size(); ++i) {
        Identifiable id1 = graph->getIDs()[i];
        double width = RandomGenerator::instance().doubleRange(0, EmbeddablePlane<T>::getWidth());
        double height = RandomGenerator::instance().doubleRange(0, EmbeddablePlane<T>::getHeight());
        auto spot = Spot<T>(width, height, graph->getValue(id1));
        this->insertSpot(spot);
        // Ensure no two points are too close initially
        for (int j = 0; j < i; ++j) {
            Identifiable id2 = graph->getIDs()[j];
            double dx = this->getSpot(id1).getX() - this->getSpot(id2).getX();
            double dy = this->getSpot(id1).getY() - this->getSpot(id2).getY();
            double dist = std::sqrt(dx*dx + dy*dy);
            if (dist < idealLength / 2) {
                assert(attempts < 1000);
                i = -1;
                this->clear();
                break;
            }
        }
    }
    currentIteration = 0;
}

template<typename T> bool EmbeddablePlane<T>::stepForceDirected(){
    if (planarLayout) return stepPlanar();
    if (currentIteration == -1){
        return false;
    }

    updateTempSpots();

    double attractiveTemperature = (1.0 - (double)currentIteration/iterationsMax) + minimumTemperature;
    double repulsionTemperature = (1.0 - (double)currentIteration/repulsionIterationsMax) + minimumTemperature;
    double magnetTemperature = ((double)currentIteration/repulsionIterationsMax) + minimumTemperature;

    applyRepulse(repulsionTemperature);
    applyAttraction(attractiveTemperature);
    applyMagnetForces(magnetTemperature);

    for (Identifiable id : this->getIDs()) {
        clampToBorder(id);
    }

    this->commitSpots();
    currentIteration++;
    if (currentIteration >= repulsionIterationsMax) {
        currentIteration = -1;
        warnEdgeIntersections();
    }
    return true;
}

template<typename T> bool EmbeddablePlane<T>::stepForceDirectedStable(){
    if (planarLayout) return stepPlanar();
    if (currentIteration == -1)
        return false;

    std::vector<Spot<T>> new_points =  EmbeddablePlane<T>::getIDs();
    double attractiveTemperature = (1.0 - (double)currentIteration/iterationsMax) + minimumTemperature;
    double repulsionTemperature = (1.0 - (double)currentIteration/repulsionIterationsMax) + minimumTemperature;
    double repulsionEdgeTemperature = (1.0 - (double)currentIteration/repulsionEdgeIterationsMax) + minimumTemperature;

    applyRepulse(repulsionTemperature, new_points);
    applyAttraction(attractiveTemperature, new_points);
    applyEdgeRepulse(repulsionEdgeTemperature, new_points);
    applyMagnetForces(repulsionTemperature);

    for (int i = 0; i < EmbeddablePlane<T>::getSpotsNumber(); ++i) {
        new_points[i].setX(std::max(0.0, std::min(EmbeddablePlane<T>::getWidth(), new_points[i].getX())));
        new_points[i].setY(std::max(0.0, std::min(EmbeddablePlane<T>::getHeight(), new_points[i].getY())));
    }

    this->commitSpots();
    currentIteration++;
    if (currentIteration >= repulsionIterationsMax) {
        currentIteration = -1;
        warnEdgeIntersections();
    }
    return true;
}

template<typename T> void EmbeddablePlane<T>::embedNode(Node<T> node, double x, double y){
    insertSpot(Spot<T>(x, y, node.getID()));
}

template<typename T> void EmbeddablePlane<T>::embed(std::shared_ptr<const Graph<T>> graph){
    initEmbed(std::move(graph));
    while (stepPlanar()) {}
}

template<typename T> bool EmbeddablePlane<T>::isEmbedding(){
    if (planarLayout) return planarLayout->isRunning();
    return currentIteration != -1;
}

template<typename T>
std::vector<EdgeIntersection> EmbeddablePlane<T>::findEdgeIntersections() const {
    if (!currentGraph) return {};

    // Normalize and sort so each undirected edge/pair is checked exactly once,
    // with reproducible diagnostics regardless of node or adjacency order.
    std::vector<std::pair<Identifiable, Identifiable>> edges;
    for (Identifiable id : currentGraph->getIDs()) {
        for (Identifiable neighbour : currentGraph->getNeighbours(id)) {
            edges.push_back(PairIDHash::normalize({id, neighbour}));
        }
    }
    std::sort(edges.begin(), edges.end());
    edges.erase(std::unique(edges.begin(), edges.end()), edges.end());

    std::vector<EdgeIntersection> intersections;
    for (size_t i = 0; i < edges.size(); ++i) {
        const auto& first = edges[i];
        const auto a = this->getSpot(first.first).getCoords();
        const auto b = this->getSpot(first.second).getCoords();
        for (size_t j = i + 1; j < edges.size(); ++j) {
            const auto& second = edges[j];
            const auto c = this->getSpot(second.first).getCoords();
            const auto d = this->getSpot(second.second).getCoords();
            const bool sharedVertex = first.first == second.first || first.first == second.second
                || first.second == second.first || first.second == second.second;

            if (planar::segmentsIntersect(a, b, c, d, sharedVertex)) {
                intersections.push_back({first, second});
            }
        }
    }
    return intersections;
}

template<typename T>
size_t EmbeddablePlane<T>::warnEdgeIntersections(std::ostream& output) const {
    const auto intersections = findEdgeIntersections();
    if (!intersections.empty()) {
        output << "Warning: embedding has " << intersections.size()
            << " intersecting edge pair(s) before rasterization/zone growth:\n";
        for (const auto& intersection : intersections) {
            output << "  Edge (" << intersection.firstEdge.first.getID() << ", "
                << intersection.firstEdge.second.getID() << ") intersects edge ("
                << intersection.secondEdge.first.getID() << ", "
                << intersection.secondEdge.second.getID() << ")\n";
        }
    }
    return intersections.size();
}

template <typename T>
void EmbeddablePlane<T>::updateTempSpots(){
    for (Identifiable id : this->getIDs()){
        temp_spots[id] = this->getSpot(id);
    }
}

template <typename T>
void EmbeddablePlane<T>::setupTempSpots(){
    temp_spots.clear();
    temp_spots.reserve(this->getSpotsNumber());
    for (Identifiable id : this->getIDs()){
        temp_spots[id] = this->getSpot(id);
    }
}

template <typename T>
void EmbeddablePlane<T>::calculateZoneRadius(){
    constexpr double shapeFactor = 3.5;
    double averageZoneArea = (this->getWidth() * this->getHeight()) / currentGraph->getSize() / shapeFactor;
    double averageZoneRadius = std::sqrt(averageZoneArea);
    idealLength = averageZoneRadius;
}


template<typename T> DoubleVector2 EmbeddablePlane<T>::getClosestPointOnEdge(const DoubleVector2 point, const DoubleVector2 end1, const DoubleVector2 end2){
    DoubleVector2 segment = {end2.x - end1.x, end2.y - end1.y};
    
    DoubleVector2 p1_to_p = {point.x - end1.x, point.y - end1.y};
    
    double segmentLengthSquared = segment.x * segment.x + segment.y * segment.y;
    
    if (segmentLengthSquared == 0) {
        return end1;
    }
    
    double t = (p1_to_p.x * segment.x + p1_to_p.y * segment.y) / segmentLengthSquared;
    
    t = std::max(0.0, std::min(1.0, t));
    
    DoubleVector2 closest = {
        end1.x + t * segment.x,
        end1.y + t * segment.y
    };
    
    return closest;
}

template<typename T> void EmbeddablePlane<T>::applyRepulse(double temperature){
    if (currentIteration > repulsionIterationsMax){
        return;
    }
    for (Identifiable id : this->getIDs()) {
        this->applyBorderRepulseToSpot(id, temperature);
        for (Identifiable id2 : this->getIDs()) {
            if (id == id2) continue;
            double dx = this->getSpot(id2).getX() - this->getSpot(id).getX();
            double dy = this->getSpot(id2).getY() - this->getSpot(id).getY();
            double distanceSquared = std::max(minDistanceRepulsion, dx*dx + dy*dy);
            
            double force = kRepulsion * temperature / (distanceSquared);
            temp_spots.at(id).changeX(-force * dx);
            temp_spots.at(id).changeY(-force * dy);
        }
    }
}

template <typename T>
void EmbeddablePlane<T>::applyBorderRepulseToSpot(Identifiable id, double temperature){
    double upperDistance = std::max(minDistanceRepulsion, this->getSpot(id).getY() - this->getUpperY());
    double upperForce = kBorderRepulsion * temperature / (upperDistance * upperDistance); 
    
    double bottomDistance = std::max(minDistanceRepulsion, this->getBottomY() - this->getSpot(id).getY());
    double bottomForce = kBorderRepulsion * temperature / (bottomDistance * bottomDistance * -1);
    
    double leftDistance = std::max(minDistanceRepulsion, this->getSpot(id).getX() - this->getLeftX());
    double leftForce = kBorderRepulsion * temperature / (leftDistance * leftDistance);
    
    double rightDistance = std::max(minDistanceRepulsion, this->getRightX() - this->getSpot(id).getX());
    double rightForce = kBorderRepulsion * temperature / (rightDistance * rightDistance * -1);

    double dx = leftForce + rightForce;
    double dy = upperForce + bottomForce;

    temp_spots.at(id).changeX(dx);
    temp_spots.at(id).changeY(dy);
}

template <typename T>
void EmbeddablePlane<T>::clampToBorder(Identifiable id){
    temp_spots.at(id).setX(std::max(0.0, std::min(this->getWidth(), temp_spots.at(id).getX())));
    temp_spots.at(id).setY(std::max(0.0, std::min(this->getHeight(), temp_spots.at(id).getY())));
}

template <typename T>
void EmbeddablePlane<T>::commitSpots(){
    this->updateSpots(temp_spots);
}

template <typename T>
void EmbeddablePlane<T>::applyEdgeRepulse(double temperature){
    if (currentIteration > repulsionEdgeIterationsMax){
        return;
    }
    for (Identifiable spot_id : this->getIDs()){
        for (Identifiable end1_id : this->getIDs()){
            if (spot_id == end1_id)
                continue;
            for (Identifiable end2_id : currentGraph->getNode(end1_id).getNeighbours()){
                if (end2_id == spot_id || end2_id >= end1_id)
                    continue;
                DoubleVector2 edgeSpot = getClosestPointOnEdge(temp_spots[spot_id].getCoords(), temp_spots[end1_id].getCoords(), temp_spots[end2_id].getCoords());
                double dx = edgeSpot.x - this->getSpot(spot_id).getX();
                double dy = edgeSpot.y - this->getSpot(spot_id).getY();
                double distanceSquared = std::max(minDistanceRepulsion, dx*dx + dy*dy);
                
                double force = kRepulsion * temperature / (distanceSquared);
                temp_spots[spot_id].changeX(-force * dx);
                temp_spots[spot_id].changeY(-force * dy);
            }
        }
    }
}

template <typename T>
void EmbeddablePlane<T>::applyMagnetForces(double temperature){
    temperature = 1.0;
    if (currentIteration < magnetForceIterationMin || currentIteration > magnetForceIterationMax)
        return;
    for (const Magnet& magnet : magnets){
        for (Identifiable id : this->getIDs()) {
            if (this->getValue(id).getSusceptibility() == 0)
                continue;
            DoubleVector2 force = magnet.calculateForce<T>(this->getSpot(id).getCoords(), this->getValue(id)) * temperature;
            temp_spots[id].changeX(force.x); 
            temp_spots[id].changeY(force.y);
        }       
    }
}

template<typename T> void EmbeddablePlane<T>::applyAttraction(double temperature){
    if (currentIteration > iterationsMax){
        return;
    }
    for (Identifiable spot_id : this->getIDs()) {
        for (Identifiable neighbor_id : currentGraph->getNode(spot_id).getNeighbours()) {
            if (neighbor_id <= spot_id) continue;
            
            double dx = this->getSpot(neighbor_id).getX() - this->getSpot(spot_id).getX();
            double dy = this->getSpot(neighbor_id).getY() - this->getSpot(spot_id).getY();
            double length = std::max(std::sqrt(dx*dx + dy*dy), minDistanceRepulsion);
            
            double force = kAttraction * temperature * (length - idealLength)/length;
            temp_spots[spot_id].changeX(force * dx);
            temp_spots[spot_id].changeY(force * dy);
            temp_spots[neighbor_id].changeX(-force * dx);
            temp_spots[neighbor_id].changeY(-force * dy);
        }
    }
}

template <typename T>
void EmbeddablePlane<T>::applyMagnetGrid(Grid<IdentifiedMagnet> magnetRelativePoses){
    magnets = magnetRelativePoses.applyToDoublePoints(this->getSize());
    for (auto magnet : magnets){
    }
}
