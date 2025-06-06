#pragma once
#include <utility>

struct Point2{
    double x = 0;
    double y = 0;
};

class Spot{
    public:
        Spot();
        Spot(double x, double y, int id);
        void setX(double x);
        void setY(double y);
        void changeX(double dx);
        void changeY(double dy);
        Point2 getCoords();
        double getX() const;
        double getY() const;
        int getID() const;
    private:
        double x;
        double y;
        int id;
};