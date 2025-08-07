#pragma once

class Radial{
    public:
        Radial(){};
        Radial(double radius) : radius{radius}{};
        double getRadius() const;
    private:
        double radius = 0;
};