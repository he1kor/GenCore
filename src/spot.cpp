#include "spot.h"

Spot::Spot(){
    x = -1;
    y = -1;
    id = -1;
}

Spot::Spot(double x, double y, int id){
    this->x = x;
    this->y = y;
    this->id = id;
}

void Spot::setX(double x){
    this->x = x;
}

void Spot::setY(double y){
    this->y = y;
}

void Spot::changeX(double dx){
    x += dx;
}

void Spot::changeY(double dy){
    y += dy;
}

double Spot::getX() const{
    return x;
}

double Spot::getY() const{
    return y;
}

int Spot::getID() const{
    return id;
}

Point2 Spot::getCoords(){
    return {x, y};
}
