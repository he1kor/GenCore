#pragma once

#include "plane.h"
#include "grid.h"
#include <stdexcept>

#include <cmath>

int doubleToIndex(double value, double size) {
    if (size < 0.0)
        throw std::invalid_argument("size must be non-negative");
    if (value >= size)
        return std::ceil(size) - 1;;
    if (value < 0.0)
        return 0;
    return static_cast<int>(std::floor(value));
}

template<typename T>
Grid<T> safeRasterizePlane(const Plane<T>& plane){
    std::vector<T> tileset;
    tileset.reserve(plane.getSpotsNumber());
    for (Identifiable id : plane.getIDs()){
        tileset.push_back(plane.getValue(id));
        DoublePoint2 point = plane.getRelativePosition(id);
    }
    Grid<T> result(ceil(plane.getWidth()), ceil(plane.getHeight()), tileset);
};