#pragma once
#include <utility>

class Spot{
    public:
        Spot();
        Spot(double x, double y, int id);
        void setX(double x);
        void setY(double y);
        void changeX(double dx);
        void changeY(double dy);
        double getX();
        double getY();
        int getID();
        std::pair<double, double> getCoords();
    private:
        double x;
        double y;
        int id;
};