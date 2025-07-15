#pragma once

#include "plane.h"
#include "grid.h"

template<typename T>
Grid<T> safeRasterizePlane(const Plane<T>& plane){
    std::vector<T> tileset;
    tileset.reserve(plane.getSpotsNumber());
    for (Identifiable id : plane.getIDs()){
        tileset.push_back(plane.getValue(id));
    }
    Grid<T> result(ceil(plane.getWidth()), ceil(plane.getHeight()), tileset);
};