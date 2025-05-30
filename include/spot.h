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
        double getX() const;
        double getY() const;
        int getID() const;
        std::pair<double, double> getCoords();
    private:
        double x;
        double y;
        int id;
};