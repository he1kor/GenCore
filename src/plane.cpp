#include "plane.h"
#include <stdexcept>

Plane::Plane(double width, double height){
    this->width = width;
    this->height = height;
}

void Plane::addSpot(Spot spot){
    if (spot.getID() > ID_LIMIT)
        throw std::runtime_error("ID limit reached");
    if (spot.getID() >= getSpotsNumber())
        spots.resize(spot.getID() + 1);
    spots.at(spot.getID()) = spot;
}

void Plane::addSpot(double x, double y){
    spots.push_back(Spot(x, y, getSpotsNumber()-1));
}

void Plane::setSpots(std::vector<Spot> spots){
    for (Spot spot : spots){
        addSpot(spot);
    }
}

double Plane::getWidth() const{
    return width;
}

double Plane::getHeight() const{
    return height;
}

int Plane::getSpotsNumber() const{
    return spots.size();
}

const std::vector<Spot> &Plane::getSpots() const{
    return spots;
}
