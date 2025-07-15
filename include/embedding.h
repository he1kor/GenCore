#pragma once



#include <iostream>
#include <vector>
#include <list>
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <stack>

#include "graph.h" 
#include "plane.h"

template<typename T>
class EmbeddablePlane : public Plane<T>{
    public:
        using Plane<T>::Plane;
        void initEmbed(const Graph<T> *graph);
        bool stepForceDirected();
        bool stepForceDirectedStable();
        void embedNode(Node<T> node, double x, double y);
        void embed(const Graph<T> *graph);
        bool isEmbedding();
        void updateTempSpots();
        void setupTempSpots();

    private:
        Point2 getClosestPointOnEdge(const Point2 point, const Point2 end1, const Point2 end2);
        void applyRepulse(double temperature);
        void applyBorderRepulseToSpot(Identifiable id, double temperature);
        void clampToBorder(Identifiable id);
        void commitSpots();
        void applyEdgeRepulse(double temperature);
        void applyAttraction(double temperature);
        int currentIteration = -1;
        const int iterationsMax = 500;
        const int repulsionEdgeIterationsMax = 500;
        const int repulsionIterationsMax = 600;
        const double minDistanceRepulsion = 2.0;
        const double kEdgeRepulsion = 5.0f;
        const double kRepulsion = 15.0;
        const double kAttraction = 0.150;
        const double idealLength = 5.0;
        const double minimumTemperature = 0.01;
        const Graph<T>* currentGraph;
        std::unordered_map<Identifiable, Spot<T>, IDHash> temp_spots;
};

#include <random>

template<typename T> void EmbeddablePlane<T>::initEmbed(const Graph<T> *graph){
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<double> distWidth(0, EmbeddablePlane<T>::getWidth());
    std::uniform_real_distribution<double> distHeight(0, EmbeddablePlane<T>::getHeight());

    //addSpot(Spot(64, 62, 0));
    //addSpot(Spot(72, 66, 1));
    //addSpot(Spot(56, 82, 2));
    //addSpot(Spot(60, 58, 3));
    //addSpot(Spot(56, 50, 4));
    // Outer pentagon (nodes 0-4)
    this->insertSpot(Spot<T>(64, 16, T(0)));    // Top-center
    this->insertSpot(Spot<T>(96, 32, T(1)));    // Upper-right
    this->insertSpot(Spot<T>(112, 80, T(2)));   // Far-right
    this->insertSpot(Spot<T>(80, 112, T(3)));   // Lower-right
    this->insertSpot(Spot<T>(32, 112, T(4)));   // Lower-left

    // Inner layer (nodes 5-9)
    this->insertSpot(Spot<T>(96, 64, T(5)));    // Right-middle
    this->insertSpot(Spot<T>(80, 48, T(6)));    // Right-upper-center
    this->insertSpot(Spot<T>(80, 80, T(7)));    // Right-lower-center
    this->insertSpot(Spot<T>(48, 80, T(8)));    // Left-lower-center
    this->insertSpot(Spot<T>(32, 64, T(9)));    // Left-middle

    // Central hub (node 10) and extensions (nodes 11-15)
    this->insertSpot(Spot<T>(64, 64, T(10)));   // Exact center
    this->insertSpot(Spot<T>(48, 48, T(11)));   // Upper-left-center
    this->insertSpot(Spot<T>(64, 32, T(12)));   // Top-inner
    this->insertSpot(Spot<T>(48, 32, T(13)));   // Upper-left
    this->insertSpot(Spot<T>(32, 48, T(14)));   // Left-upper-center
    this->insertSpot(Spot<T>(32, 16, T(15)));   // Far-left-top

    // 1. Random initial placement with minimum distance
    //for (int i = 0; i < graph->size(); ++i) {
    //    bool valid = false;
    //    while (!valid) {
    //        addSpot(Spot(distWidth(rng), distHeight(rng), i));
    //        valid = true;
    //        // Ensure no two points are too close initially
    //        for (int j = 0; j < i; ++j) {
    //            double dx = getSpots()[j].getX() - getSpots()[i].getX();
    //            double dy = getSpots()[j].getY() - getSpots()[i].getY();
    //            double dist = std::sqrt(dx*dx + dy*dy);
    //            if (dist < idealLength / 2) {
    //                valid = false;
    //                break;
    //            }
    //        }
    //    }
    //}
    this->currentGraph = graph;
    currentIteration = 0;
}

template<typename T> bool EmbeddablePlane<T>::stepForceDirected(){
    if (currentIteration == -1)
        return false;

    updateTempSpots();

    double attractiveTemperature = (1.0 - (double)currentIteration/iterationsMax) + minimumTemperature;
    double repulsionTemperature = (1.0 - (double)currentIteration/repulsionIterationsMax) + minimumTemperature;

    applyRepulse(repulsionTemperature);
    applyAttraction(attractiveTemperature);    

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
    addSpot(Spot(x, y, node.getID()));
}

template<typename T> void EmbeddablePlane<T>::embed(const Graph<T> *graph){
    
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

template<typename T> Point2 EmbeddablePlane<T>::getClosestPointOnEdge(const Point2 point, const Point2 end1, const Point2 end2){
    Point2 segment = {end2.x - end1.x, end2.y - end1.y};
    
    Point2 p1_to_p = {point.x - end1.x, point.y - end1.y};
    
    double segmentLengthSquared = segment.x * segment.x + segment.y * segment.y;
    
    if (segmentLengthSquared == 0) {
        return end1;
    }
    
    double t = (p1_to_p.x * segment.x + p1_to_p.y * segment.y) / segmentLengthSquared;
    
    t = std::max(0.0, std::min(1.0, t));
    
    Point2 closest = {
        end1.x + t * segment.x,
        end1.y + t * segment.y
    };
    
    return closest;
}

template<typename T> void EmbeddablePlane<T>::applyRepulse(double temperature)
{
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
    double upperForce = kRepulsion * temperature / (upperDistance * upperDistance); 
    
    double bottomDistance = std::max(minDistanceRepulsion, this->getBottomY() - this->getSpot(id).getY());
    double bottomForce = kRepulsion * temperature / (bottomDistance * bottomDistance * -1);
    
    double leftDistance = std::max(minDistanceRepulsion, this->getSpot(id).getX() - this->getLeftX());
    double leftForce = kRepulsion * temperature / (leftDistance * leftDistance);
    
    double rightDistance = std::max(minDistanceRepulsion, this->getRightX() - this->getSpot(id).getX());
    double rightForce = kRepulsion * temperature / (rightDistance * rightDistance * -1);

    temp_spots.at(id).changeX(leftForce + rightForce);
    temp_spots.at(id).changeY(bottomForce + upperForce);
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
                Point2 edgeSpot = getClosestPointOnEdge(temp_spots[spot_id].getCoords(), temp_spots[end1_id].getCoords(), temp_spots[end2_id].getCoords());
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
