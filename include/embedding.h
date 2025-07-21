#pragma once



#include <iostream>
#include <vector>
#include <list>
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <stack>
#include <cassert>

#include "math_util.h"
#include "graph.h" 
#include "plane.h"
#include "grid.h"

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


template<typename T>
class EmbeddablePlane : public Plane<T>{
    public:
        using Plane<T>::Plane;
        void initEmbed(std::shared_ptr<const Graph<T>> graph);
        bool stepForceDirected();
        bool stepForceDirectedStable();
        void embedNode(Node<T> node, double x, double y);
        void embed(std::shared_ptr<const Graph<T>> graph);
        bool isEmbedding();
        void updateTempSpots();
        void setupTempSpots();
        void calculateZoneRadius();
        void applyMagnetGrid(Grid<IdentifiedMagnet> magnetRelativePoses);

    private:
        DoubleVector2 getClosestPointOnEdge(const DoubleVector2 point, const DoubleVector2 end1, const DoubleVector2 end2);
        void applyRepulse(double temperature);
        void applyBorderRepulseToSpot(Identifiable id, double temperature);
        void clampToBorder(Identifiable id);
        void commitSpots();
        void applyEdgeRepulse(double temperature);
        void applyMagnetForces(double temperature);
        void applyAttraction(double temperature);

        
        int currentIteration = -1;
        double idealLength = 5.0;
        
        std::shared_ptr<const Graph<T>> currentGraph;
        std::unordered_map<Identifiable, Spot<T>, IDHash> temp_spots;
        std::vector<IdentifiedMagnet> magnets;
        
        const int iterationsMax = 100;
        const int repulsionEdgeIterationsMax = 100;
        const int repulsionIterationsMax = 120;
        const double minDistanceRepulsion = 2.0;
        const double kEdgeRepulsion = 30.0f;
        const double kRepulsion = 17.0;
        const double kBorderRepulsion = 57.0;
        const double kAttraction = 0.150;
        const bool squareDistance = true;
        const double minimumTemperature = 0.01;
        const double magnetForceIterationMin = 20;
        const double magnetForceIterationMax = 120;
};

#include <random>

template<typename T> void EmbeddablePlane<T>::initEmbed(std::shared_ptr<const Graph<T>> graph){
    this->currentGraph = graph;
    calculateZoneRadius();
    this->clear();
    
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<double> distWidth(0, EmbeddablePlane<T>::getWidth());
    std::uniform_real_distribution<double> distHeight(0, EmbeddablePlane<T>::getHeight());
    
    int attempts = 0;
    for (int i = 0; i < graph->size(); ++i) {
        Identifiable id1 = graph->getIDs()[i];
        auto spot = Spot<T>(distWidth(rng), distHeight(rng), graph->getValue(id1));
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
    if (currentIteration >= repulsionIterationsMax)
        currentIteration = -1;
    return true;
}

template<typename T> bool EmbeddablePlane<T>::stepForceDirectedStable(){
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
    if (currentIteration >= repulsionIterationsMax)
        currentIteration = -1;
    return true;
}

template<typename T> void EmbeddablePlane<T>::embedNode(Node<T> node, double x, double y){
    insertSpot(Spot<T>(x, y, node.getID()));
}

template<typename T> void EmbeddablePlane<T>::embed(std::shared_ptr<const Graph<T>> graph){
    
}

template<typename T> bool EmbeddablePlane<T>::isEmbedding(){
    return currentIteration != -1;
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
            if (this->getValue(id).getSuspectibility() == 0)
                continue;
            DoubleVector2 force = magnet.calculateForce<T>(this->getSpot(id).getCoords(), this->getValue(id)) * temperature;
            temp_spots[id].changeX(force.x); 
            temp_spots[id].changeX(force.y); 
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
