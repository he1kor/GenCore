#pragma once
#include <vector>
#include "spot.h"

constexpr int ID_LIMIT = 1000;

class Plane{
    public:
        Plane(double width, double height);
        void addSpot(Spot spot);
        void setSpots(std::vector<Spot> spots);
        double getWidth();
        double getHeight();
        int getSpotsNumber();
        const std::vector<Spot>& getSpots();
    private:
        std::vector<Spot> spots;
        double width;
        double height;
};