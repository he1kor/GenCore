#include "embedding.h"
#include <random>
#include <iostream>

void EmbeddablePlane::initEmbed(const Graph *graph){
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<double> distWidth(0, getWidth());
    std::uniform_real_distribution<double> distHeight(0, getHeight());

    addSpot(Spot(64, 62, 0));
    addSpot(Spot(72, 66, 1));
    addSpot(Spot(56, 82, 2));
    addSpot(Spot(60, 58, 3));
    addSpot(Spot(56, 50, 4));

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
    for (auto spot : getSpots()){
        std::cout << "spot:\n";
        std::cout << spot.getX() << "\t" << spot.getY() << '\n';
    }
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

void EmbeddablePlane::embedNode(Node node, double x, double y){
    addSpot(Spot(x, y, node.getID()));
}

bool EmbeddablePlane::isEmbedding(){
    return currentIteration != -1;
}

void EmbeddablePlane::applyRepulse(double temperature, std::vector<Spot>& spots){
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

        if (currentIteration == 0)
        std::cout << i << '\t' << leftForce + rightForce << '\t' << bottomForce + upperForce << '\n';

        spots[i].changeX(leftForce + rightForce);
        spots[i].changeY(bottomForce + upperForce);


        for (int j = 0; j < getSpotsNumber(); ++j) {
            if (i == j) continue;
            double dx = getSpots()[j].getX() - getSpots()[i].getX();
            double dy = getSpots()[j].getY() - getSpots()[i].getY();
            double distance = std::max(minDistanceRepulsion, std::sqrt(dx*dx + dy*dy));
            
            double force = kRepulsion * temperature / (distance * distance);
            spots[i].changeX(-force * dx);
            spots[i].changeY(-force * dy);
        }
    }
}

void EmbeddablePlane::applyAttraction(double temperature, std::vector<Spot> &spots){
    if (currentIteration <= iterationsMax){
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
}
