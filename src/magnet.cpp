#include "magnet.h"

#include <cmath>

Magnet::Magnet(double force) : SafeDoubleVector2{0.0, 0.0}, force{force}{}

Magnet::Magnet(double x, double y, double force) : SafeDoubleVector2{x, y}, force{force} {}

Magnet::Magnet(DoubleVector2 point2, double force) : SafeDoubleVector2{point2}, force{force} {}

void Magnet::setPos(DoubleVector2 point2){
    this->x = point2.x;
    this->y = point2.x;
}

DoubleVector2 Magnet::calculateForce(const DoubleVector2& object) const{
    return calculateForce(object, Magnet::defaultRepulseForce);
}

void Magnet::setForceFunction(std::function<double(double)> distanceToForce){
    this->distanceToForce = distanceToForce;
}

DoubleVector2 Magnet::calculateForce(const DoubleVector2& object, double susceptibility) const{
    double dx = object.x - x;
    double dy = object.y - y;
    double distance = std::sqrt(dx * dx + dy * dy);
    double appliedForce = force * susceptibility * distanceToForce(distance);
    return {dx * appliedForce, dy * appliedForce};
}
