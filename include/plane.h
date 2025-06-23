#pragma once
#include <vector>
#include "spot.h"

constexpr int ID_LIMIT = 1000;

class Plane{
    public:
        Plane(double width, double height);
        void addSpot(Spot spot);
        void addSpot(double x, double y);
        void setSpots(std::vector<Spot> spots);
        void setUpperY(double upperY);
        void setLeftX(double leftX);
        double getUpperY();
        double getBottomY();
        double getLeftX();
        double getRightX();
        void clear();
        double getWidth() const;
        double getHeight() const;
        int getSpotsNumber() const;
        Spot getSpot(int i);
        const std::vector<Spot>& getSpots() const;
    private:
        std::vector<Spot> spots;
        double leftX = 0;
        double upperY = 0;
        double width;
        double height;
};