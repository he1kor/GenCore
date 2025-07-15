#pragma once
#include <vector>
#include <stdexcept>
#include "spot.h"

constexpr int ID_LIMIT = 1000;

template<typename T>
class Plane {
public:
    static_assert(std::is_base_of_v<Identifiable, T>, "T must inherit from Identifiable");
    Plane(double width, double height) : width(width), height(height) {}
    
    void insertSpot(Spot<T> spot) {
        spots[spot.getIdentifiable().getID()] = spot;
        ids.push_back(spot.getIdentifiable());
    }
    
    void addDummySpot(double x, double y) {
        spots[dummy] = (Spot<T>(x, y));
        ids.push_back(dummy);
        dummy++;
    }
    
    void setSpots(const std::vector<Spot<T>>& spots){
        clear();
        insertSpots(spots);
    }
    void updateSpots(const std::unordered_map<Identifiable, Spot<T>, IDHash>& spots){
        this->spots = spots;
    }

    void insertSpots(const std::vector<Spot<T>>& spots) {
        for (const Spot<T>& spot : spots) {
            insertSpot(spot);
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
        ids.clear();
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
    double getRelativeX(Identifiable id) const{
        return getSpot(id).getX() - getLeftX();
    }
    
    double getRelativeY(Identifiable id) const{
        return getSpot(id).getY() - getUpperY();
    }
    DoublePoint2 getRelativePosition(Identifiable id) const{
        return DoublePoint2(getRelativeX(id), getRelativeY(id));
    }

    const Spot<T>& getSpot(Identifiable id) const {
        return spots.at(id);
    }
    const T& getValue(Identifiable id) const{
        return getSpot(id).getValue();
    }
    
    const std::vector<Identifiable>& getIDs() const {
        return ids;
    }

private:
    std::unordered_map<Identifiable,Spot<T>,IDHash> spots;
    std::vector<Identifiable> ids;
    double leftX = 0;
    double upperY = 0;
    double width;
    double height;
    int dummy = -10000;
};