#include "spot.h"

Spot::Spot(double x, double y, int id){
    this->x = x;
    this->y = y;
    this->id = id;
}

double Spot::getX(){
    return x;
}

double Spot::getY(){
    return y;
}

int Spot::getID(){
    return id;
}

std::pair<double, double> Spot::getCoords(){
    return {x, y};
}
