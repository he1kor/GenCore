#include "magnet.h"

#include <cmath>

Magnet::Magnet(double force) : DoubleVector2{0, 0}{}

Magnet::Magnet(double x, double y, double force) : DoubleVector2{x, y}, force{force} {}

Magnet::Magnet(DoubleVector2 point2, double force) : DoubleVector2{point2}, force{force} {}

DoubleVector2 Magnet::getRepulse(DoubleVector2 object){
    return getForce(object, Magnet::defaultRepulseForce);
}

DoubleVector2 Magnet::getAttract(DoubleVector2 object){
    return getForce(object, Magnet::defaultAttractForce);
}

void Magnet::setPos(DoubleVector2 point2){
    this->x = point2.x;
    this->y = point2.x;
}


void Magnet::setForceFunction(std::function<double(double)> distanceToForce){
    this->distanceToForce = distanceToForce;
}

DoubleVector2 Magnet::getForce(DoubleVector2 object, double susceptibility){
    double dx = object.x - x;
    double dy = object.y - y;
    double distance = std::sqrt(dx * dx + dy * dy);
    double force = susceptibility * distanceToForce(distance);
    return {dx * force, dy * force};
}
