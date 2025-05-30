#pragma once
#include <utility>

class Spot{
    public:
        Spot();
        Spot(double x, double y, int id);
        double getX();
        double getY();
        int getID();
        std::pair<double, double> getCoords();
    private:
        double x;
        double y;
        int id;
};