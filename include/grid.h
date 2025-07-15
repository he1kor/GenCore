#pragma once

#include <vector>
#include <unordered_map>

#include "2d.h"
#include "identifiable.h"

template<typename T>
class Grid{
    static_assert(std::is_base_of_v<Identifiable, T>, "T must inherit from Identifiable");
    public:
        Grid(int width, int height);

        template<std::ranges::input_range R>
        requires std::convertible_to<std::ranges::range_value_t<R>, T>
        Grid(int width, int height, R&& range);

        void setTile(int x, int y, Identifiable value);
        void setTile(IntPoint2 point, Identifiable value);
        Identifiable getTile(int x, int y);
        int getWidth();
        int getHeight();
    private:
        int width;
        int height;
        std::vector<std::vector<Identifiable>> matrix;
        std::unordered_map<Identifiable, T, IDHash> tileset;
};

template <typename T> 
Grid<T>::Grid(int width, int height) : width(width), height(height){
    matrix = std::vector<std::vector<Identifiable>>(height, std::vector(width));
}

template <typename T>
void Grid<T>::setTile(int x, int y, Identifiable value){
    matrix.at(y).at(x) = value;
}

template <typename T>
void Grid<T>::setTile(IntPoint2 point, Identifiable value){
    matrix.at(point.y).at(point.x) = value;
}

template <typename T>
Identifiable Grid<T>::getTile(int x, int y){
    return matrix.at(y).at(x);
}

template <typename T>
int Grid<T>::getWidth(){
    return width;
}

template <typename T>
int Grid<T>::getHeight(){
    return height;
}

template <typename T> 
template <std::ranges::input_range R>
requires std::convertible_to<std::ranges::range_value_t<R>, T>
Grid<T>::Grid(int width, int height, R&& range) : Grid(width, height){
    for (const T& t : range) {
        tileset[static_cast<Identifiable>(t)] = t;
    }
}