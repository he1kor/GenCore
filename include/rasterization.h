#pragma once

#include <stdexcept>
#include <cmath>

#include "plane.h"
#include "grid.h"
#include "2d.h"

int rasterizeSize(double size){
    return std::ceil(size);
}

int doubleToIndex(double value, double size) {
    if (size < 0.0)
        throw std::invalid_argument("size must be non-negative");
    if (value >= size)
        return rasterizeSize(size) - 1;;
    if (value < 0.0)
        return 0;
    return static_cast<int>(std::floor(value));
}

IntVector2 rasterizePoint2(const DoubleVector2& DoubleVector2, double width, double height){
    return {
        doubleToIndex(DoubleVector2.x, width),
        doubleToIndex(DoubleVector2.y, height)
    };
}

template<typename T>
Grid<T> safeRasterizePlane(const Plane<T>& plane){
    std::vector<T> tileset;
    tileset.reserve(plane.getSpotsNumber());
    for (Identifiable id : plane.getIDs()){
        tileset.push_back(plane.getValue(id));
    }
    Grid<T> result(rasterizeSize(plane.getWidth()), rasterizeSize(plane.getHeight()), tileset);
    for (Identifiable id : result.getTileIDs()){
        result.setTile(rasterizePoint2(plane.getRelativePosition(id), result.getWidth(), result.getHeight()), id);
    }
    return result;
};