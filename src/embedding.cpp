#include "embedding.h"
#include <random>
#include <iostream>

void EmbeddablePlane::initEmbed(const Graph *graph){
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<double> distWidth(0, getWidth());
    std::uniform_real_distribution<double> distHeight(0, getHeight());

    //addSpot(Spot(64, 62, 0));
    //addSpot(Spot(72, 66, 1));
    //addSpot(Spot(56, 82, 2));
    //addSpot(Spot(60, 58, 3));
    //addSpot(Spot(56, 50, 4));
    // Outer pentagon (nodes 0-4)
    addSpot(Spot(64, 16, 0));    // Top-center
    addSpot(Spot(96, 32, 1));    // Upper-right
    addSpot(Spot(112, 80, 2));   // Far-right
    addSpot(Spot(80, 112, 3));   // Lower-right
    addSpot(Spot(32, 112, 4));   // Lower-left

    // Inner layer (nodes 5-9)
    addSpot(Spot(96, 64, 5));    // Right-middle
    addSpot(Spot(80, 48, 6));    // Right-upper-center
    addSpot(Spot(80, 80, 7));    // Right-lower-center
    addSpot(Spot(48, 80, 8));    // Left-lower-center
    addSpot(Spot(32, 64, 9));    // Left-middle

    // Central hub (node 10) and extensions (nodes 11-15)
    addSpot(Spot(64, 64, 10));   // Exact center
    addSpot(Spot(48, 48, 11));   // Upper-left-center
    addSpot(Spot(64, 32, 12));   // Top-inner
    addSpot(Spot(48, 32, 13));   // Upper-left
    addSpot(Spot(32, 48, 14));   // Left-upper-center
    addSpot(Spot(32, 16, 15));   // Far-left-top

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

bool EmbeddablePlane::stepForceDirected(){
    if (currentIteration == -1)
        return false;

    std::vector<Spot> new_points = getSpots();
    double attractiveTemperature = (1.0 - (double)currentIteration/iterationsMax) + minimumTemperature;
    double repulsionTemperature = (1.0 - (double)currentIteration/repulsionIterationsMax) + minimumTemperature;

    applyRepulse(repulsionTemperature, new_points);
    applyAttraction(attractiveTemperature, new_points);    

    for (int i = 0; i < getSpotsNumber(); ++i) {
        new_points[i].setX(std::max(0.0, std::min(getWidth(), new_points[i].getX())));
        new_points[i].setY(std::max(0.0, std::min(getHeight(), new_points[i].getY())));
    }

    setSpots(new_points);
    currentIteration++;
    if (currentIteration >= repulsionIterationsMax)
        currentIteration = -1;
    return true;
}

bool EmbeddablePlane::stepForceDirectedStable(){
    if (currentIteration == -1)
        return false;

    std::vector<Spot> new_points = getSpots();
    double attractiveTemperature = (1.0 - (double)currentIteration/iterationsMax) + minimumTemperature;
    double repulsionTemperature = (1.0 - (double)currentIteration/repulsionIterationsMax) + minimumTemperature;
    double repulsionEdgeTemperature = (1.0 - (double)currentIteration/repulsionEdgeIterationsMax) + minimumTemperature;

    applyRepulse(repulsionTemperature, new_points);
    applyAttraction(attractiveTemperature, new_points);
    applyEdgeRepulse(repulsionEdgeTemperature, new_points);

    for (int i = 0; i < getSpotsNumber(); ++i) {
        new_points[i].setX(std::max(0.0, std::min(getWidth(), new_points[i].getX())));
        new_points[i].setY(std::max(0.0, std::min(getHeight(), new_points[i].getY())));
    }

    setSpots(new_points);
    currentIteration++;
    if (currentIteration >= repulsionIterationsMax)
        currentIteration = -1;
    return true;
}

void EmbeddablePlane::embedNode(Node node, double x, double y){
    addSpot(Spot(x, y, node.getID()));
}

void EmbeddablePlane::embed(const Graph *graph){
    
}

bool EmbeddablePlane::isEmbedding(){
    return currentIteration != -1;
}

Point2 EmbeddablePlane::getClosestPointOnEdge(const Point2 point, const Point2 end1, const Point2 end2){
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

void EmbeddablePlane::applyRepulse(double temperature, std::vector<Spot> &spots)
{
    if (currentIteration > repulsionIterationsMax){
        return;
    }
    for (int i = 0; i < getSpotsNumber(); ++i) {
        // repel from map borders
        double upperDistance = std::max(minDistanceRepulsion, getSpots()[i].getY() - getUpperY());
        double upperForce = kRepulsion * temperature / (upperDistance * upperDistance);
        
        double bottomDistance = std::max(minDistanceRepulsion, getBottomY() - getSpots()[i].getY());
        double bottomForce = kRepulsion * temperature / (bottomDistance * bottomDistance * -1);
        
        double leftDistance = std::max(minDistanceRepulsion, getSpots()[i].getX() - getLeftX());
        double leftForce = kRepulsion * temperature / (leftDistance * leftDistance);
        
        double rightDistance = std::max(minDistanceRepulsion, getRightX() - getSpots()[i].getX());
        double rightForce = kRepulsion * temperature / (rightDistance * rightDistance * -1);

        spots[i].changeX(leftForce + rightForce);
        spots[i].changeY(bottomForce + upperForce);


        for (int j = 0; j < getSpotsNumber(); ++j) {
            if (i == j) continue;
            double dx = getSpots()[j].getX() - getSpots()[i].getX();
            double dy = getSpots()[j].getY() - getSpots()[i].getY();
            double distanceSquared = std::max(minDistanceRepulsion, dx*dx + dy*dy);
            
            double force = kRepulsion * temperature / (distanceSquared);
            spots[i].changeX(-force * dx);
            spots[i].changeY(-force * dy);
        }
    }
}

void EmbeddablePlane::applyEdgeRepulse(double temperature, std::vector<Spot> &spots){
    if (currentIteration > repulsionEdgeIterationsMax){
        return;
    }
    for (int i = 0; i < getSpotsNumber(); i++){
        for (int end1 = 0; end1 < getSpotsNumber(); end1++){
            if (end1 == i)
                continue;
            for (int end2 : currentGraph->getNode(end1).getNeighbours()){
                if (end2 == i || end2 > end1)
                    continue;
                Point2 edgeSpot = getClosestPointOnEdge(spots[i].getCoords(), spots[end1].getCoords(), spots[end2].getCoords());
                double dx = edgeSpot.x - getSpots()[i].getX();
                double dy = edgeSpot.y - getSpots()[i].getY();
                double distanceSquared = std::max(minDistanceRepulsion, dx*dx + dy*dy);
                
                double force = kRepulsion * temperature / (distanceSquared);
                spots[i].changeX(-force * dx);
                spots[i].changeY(-force * dy);
            }
        }
    }
}

void EmbeddablePlane::applyAttraction(double temperature, std::vector<Spot> &spots){
    if (currentIteration > iterationsMax){
        return;
    }
    for (int i = 0; i < getSpotsNumber(); ++i) {
            for (int neighbor : currentGraph->getNode(i).getNeighbours()) {
                if (neighbor <= i) continue;
                
                double dx = getSpots()[neighbor].getX() - getSpots()[i].getX();
                double dy = getSpots()[neighbor].getY() - getSpots()[i].getY();
                double length = std::max(std::sqrt(dx*dx + dy*dy), minDistanceRepulsion);
                
                double force = kAttraction * temperature * (length - idealLength)/length;
                spots[i].changeX(force * dx);
                spots[i].changeY(force * dy);
                spots[neighbor].changeX(-force * dx);
                spots[neighbor].changeY(-force * dy);
            }
        }
}
