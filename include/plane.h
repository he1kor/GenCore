#pragma once
#include <vector>
#include <stdexcept>
#include "spot.h"

constexpr int ID_LIMIT = 1000;

template<typename T>
class Plane {
public:
    Plane(double width, double height) : width(width), height(height) {}
    
    void addSpot(Spot<T> spot) {
        if (spot.getID() > ID_LIMIT)
            throw std::runtime_error("ID limit reached");
        if (spot.getID() >= getSpotsNumber())
            spots.resize(spot.getID() + 1);
        spots.at(spot.getID()) = spot;
    }
    
    void addSpot(double x, double y) {
        spots.push_back(Spot<T>(x, y, getSpotsNumber()));
    }
    
    void setSpots(std::vector<Spot<T>> spots) {
        for (Spot<T> spot : spots) {
            addSpot(spot);
        }
    }
    
    void setUpperY(double upperY) {
        this->upperY = upperY;
    }
    
    void setLeftX(double leftX) {
        this->leftX = leftX;
    }
    
    double getUpperY() {
        return upperY;
    }
    
    double getBottomY() {
        return upperY + height;
    }
    
    double getLeftX() {
        return leftX;
    }
    
    double getRightX() {
        return leftX + width;
    }
    
    void clear() {
        spots.clear();
    }
    
    double getWidth() const {
        return width;
    }
    
    double getHeight() const {
        return height;
    }
    
    int getSpotsNumber() const {
        return spots.size();
    }
    
    Spot<T> getSpot(int i) {
        return spots.at(i);
    }
    
    const std::vector<Spot<T>>& getSpots() const {
        return spots;
    }

private:
    std::vector<Spot<T>> spots;
    double leftX = 0;
    double upperY = 0;
    double width;
    double height;
};