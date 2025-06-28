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
        void initEmbed(const Graph *graph);
        bool stepForceDirected();
        bool stepForceDirectedStable();
        void embedNode(Node node, double x, double y);
        void embed(const Graph *graph);
        bool isEmbedding();
    private:
        Point2 getClosestPointOnEdge(const Point2 point, const Point2 end1, const Point2 end2);
        void applyRepulse(double temperature, std::vector<Spot<T>>& spots);
        void applyEdgeRepulse(double temperature, std::vector<Spot<T>>& spots);
        void applyAttraction(double temperature, std::vector<Spot<T>>& spots);
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
        const Graph* currentGraph;
};

#include <random>

template<typename T> void EmbeddablePlane<T>::initEmbed(const Graph *graph){
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
    addSpot(Spot<T>(64, 16, 0));    // Top-center
    addSpot(Spot<T>(96, 32, 1));    // Upper-right
    addSpot(Spot<T>(112, 80, 2));   // Far-right
    addSpot(Spot<T>(80, 112, 3));   // Lower-right
    addSpot(Spot<T>(32, 112, 4));   // Lower-left

    // Inner layer (nodes 5-9)
    addSpot(Spot<T>(96, 64, 5));    // Right-middle
    addSpot(Spot<T>(80, 48, 6));    // Right-upper-center
    addSpot(Spot<T>(80, 80, 7));    // Right-lower-center
    addSpot(Spot<T>(48, 80, 8));    // Left-lower-center
    addSpot(Spot<T>(32, 64, 9));    // Left-middle

    // Central hub (node 10) and extensions (nodes 11-15)
    addSpot(Spot<T>(64, 64, 10));   // Exact center
    addSpot(Spot<T>(48, 48, 11));   // Upper-left-center
    addSpot(Spot<T>(64, 32, 12));   // Top-inner
    addSpot(Spot<T>(48, 32, 13));   // Upper-left
    addSpot(Spot<T>(32, 48, 14));   // Left-upper-center
    addSpot(Spot<T>(32, 16, 15));   // Far-left-top

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

    std::vector<Spot<T>> new_points =  EmbeddablePlane<T>::getSpots();
    double attractiveTemperature = (1.0 - (double)currentIteration/iterationsMax) + minimumTemperature;
    double repulsionTemperature = (1.0 - (double)currentIteration/repulsionIterationsMax) + minimumTemperature;

    applyRepulse(repulsionTemperature, new_points);
    applyAttraction(attractiveTemperature, new_points);    

    for (int i = 0; i <  EmbeddablePlane<T>::getSpotsNumber(); ++i) {
        new_points[i].setX(std::max(0.0, std::min(EmbeddablePlane<T>::getWidth(), new_points[i].getX())));
        new_points[i].setY(std::max(0.0, std::min(EmbeddablePlane<T>::getHeight(), new_points[i].getY())));
    }

    setSpots(new_points);
    currentIteration++;
    if (currentIteration >= repulsionIterationsMax)
        currentIteration = -1;
    return true;
}

template<typename T> bool EmbeddablePlane<T>::stepForceDirectedStable(){
    if (currentIteration == -1)
        return false;

    std::vector<Spot<T>> new_points =  EmbeddablePlane<T>::getSpots();
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

    EmbeddablePlane<T>::setSpots(new_points);
    currentIteration++;
    if (currentIteration >= repulsionIterationsMax)
        currentIteration = -1;
    return true;
}

template<typename T> void EmbeddablePlane<T>::embedNode(Node node, double x, double y){
    addSpot(Spot(x, y, node.getID()));
}

template<typename T> void EmbeddablePlane<T>::embed(const Graph *graph){
    
}

template<typename T> bool EmbeddablePlane<T>::isEmbedding(){
    return currentIteration != -1;
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

template<typename T> void EmbeddablePlane<T>::applyRepulse(double temperature, std::vector<Spot<T>> &spots)
{
    if (currentIteration > repulsionIterationsMax){
        return;
    }
    for (int i = 0; i < EmbeddablePlane<T>::getSpotsNumber(); ++i) {
        // repel from map borders
        double upperDistance = std::max(minDistanceRepulsion, EmbeddablePlane<T>::getSpots()[i].getY() - EmbeddablePlane<T>::getUpperY());
        double upperForce = kRepulsion * temperature / (upperDistance * upperDistance);
        
        double bottomDistance = std::max(minDistanceRepulsion, EmbeddablePlane<T>::getBottomY() - EmbeddablePlane<T>::getSpots()[i].EmbeddablePlane<T>::getY());
        double bottomForce = kRepulsion * temperature / (bottomDistance * bottomDistance * -1);
        
        double leftDistance = std::max(minDistanceRepulsion, EmbeddablePlane<T>::getSpots()[i].getX() - EmbeddablePlane<T>::getLeftX());
        double leftForce = kRepulsion * temperature / (leftDistance * leftDistance);
        
        double rightDistance = std::max(minDistanceRepulsion, EmbeddablePlane<T>::getRightX() - EmbeddablePlane<T>::getSpots()[i].EmbeddablePlane<T>::getX());
        double rightForce = kRepulsion * temperature / (rightDistance * rightDistance * -1);

        spots[i].changeX(leftForce + rightForce);
        spots[i].changeY(bottomForce + upperForce);


        for (int j = 0; j < EmbeddablePlane<T>::getSpotsNumber(); ++j) {
            if (i == j) continue;
            double dx = EmbeddablePlane<T>::getSpots()[j].getX() - EmbeddablePlane<T>::getSpots()[i].getX();
            double dy = EmbeddablePlane<T>::getSpots()[j].getY() - EmbeddablePlane<T>::getSpots()[i].getY();
            double distanceSquared = std::max(minDistanceRepulsion, dx*dx + dy*dy);
            
            double force = kRepulsion * temperature / (distanceSquared);
            spots[i].changeX(-force * dx);
            spots[i].changeY(-force * dy);
        }
    }
}

template<typename T> void EmbeddablePlane<T>::applyEdgeRepulse(double temperature, std::vector<Spot<T>> &spots){
    if (currentIteration > repulsionEdgeIterationsMax){
        return;
    }
    for (int i = 0; i < EmbeddablePlane<T>::getSpotsNumber(); i++){
        for (int end1 = 0; end1 < EmbeddablePlane<T>::getSpotsNumber(); end1++){
            if (end1 == i)
                continue;
            for (int end2 : currentGraph->getNode(end1).getNeighbours()){
                if (end2 == i || end2 > end1)
                    continue;
                Point2 edgeSpot = getClosestPointOnEdge(spots[i].getCoords(), spots[end1].getCoords(), spots[end2].getCoords());
                double dx = edgeSpot.x - EmbeddablePlane<T>::getSpots()[i].getX();
                double dy = edgeSpot.y - EmbeddablePlane<T>::getSpots()[i].getY();
                double distanceSquared = std::max(minDistanceRepulsion, dx*dx + dy*dy);
                
                double force = kRepulsion * temperature / (distanceSquared);
                spots[i].changeX(-force * dx);
                spots[i].changeY(-force * dy);
            }
        }
    }
}

template<typename T> void EmbeddablePlane<T>::applyAttraction(double temperature, std::vector<Spot<T>> &spots){
    if (currentIteration > iterationsMax){
        return;
    }
    for (int i = 0; i < EmbeddablePlane<T>::getSpotsNumber(); ++i) {
            for (int neighbor : currentGraph->getNode(i).getNeighbours()) {
                if (neighbor <= i) continue;
                
                double dx = EmbeddablePlane<T>::getSpots()[neighbor].getX() - EmbeddablePlane<T>::getSpots()[i].getX();
                double dy = EmbeddablePlane<T>::getSpots()[neighbor].getY() - EmbeddablePlane<T>::getSpots()[i].getY();
                double length = std::max(std::sqrt(dx*dx + dy*dy), minDistanceRepulsion);
                
                double force = kAttraction * temperature * (length - idealLength)/length;
                spots[i].changeX(force * dx);
                spots[i].changeY(force * dy);
                spots[neighbor].changeX(-force * dx);
                spots[neighbor].changeY(-force * dy);
            }
        }
}
