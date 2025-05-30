#pragma once
#include <vector>
#include "spot.h"

constexpr int ID_LIMIT = 1000;

class Plane{
    public:
        Plane(double width, double height);
        void addSpot(Spot spot);
        void setSpots(std::vector<Spot> spots);
        double getWidth() const;
        double getHeight() const;
        int getSpotsNumber() const;
        const std::vector<Spot>& getSpots() const;
    private:
        std::vector<Spot> spots;
        double width;
        double height;
};