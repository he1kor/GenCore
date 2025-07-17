#pragma once

#include <vector>
#include <unordered_map>
#include <optional>

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
        Identifiable getTileID(int x, int y) const;
        const std::vector<Identifiable>& getTileIDs() const;
        T getTile(int x, int y) const;
        int getWidth() const;
        int getHeight() const;
        
        bool isEmpty(int x, int y);
        bool isEmpty(IntPoint2 point);

        bool isValidPoint(IntPoint2 point2);

        std::optional<Identifiable>tryGetID(IntPoint2 point2);
        std::optional<T>tryGetTile(IntPoint2 point2);

    private:
        int width;
        int height;
        std::vector<std::vector<Identifiable>> matrix;
        std::unordered_map<Identifiable, T, IDHash> tileset;
        std::vector<Identifiable> tileIDs;
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
Identifiable Grid<T>::getTileID(int x, int y) const{
    return matrix.at(y).at(x);
}

template <typename T>
const std::vector<Identifiable>& Grid<T>::getTileIDs() const{
    return tileIDs;
}

template <typename T>
T Grid<T>::getTile(int x, int y) const{
    return tileset.at(matrix.at(y).at(x));
}

template <typename T>
int Grid<T>::getWidth() const{
    return width;
}

template <typename T>
int Grid<T>::getHeight() const{
    return height;
}

template <typename T>
bool Grid<T>::isEmpty(int x, int y){
    return matrix.at(y).at(x) == Identifiable::nullID;
}

template <typename T>
bool Grid<T>::isEmpty(IntPoint2 point){
    return isEmpty(point.x, point.y);
}

template <typename T>
bool Grid<T>::isValidPoint(IntPoint2 point){
    if (point.x < 0 || point.y < 0)
        return false;
    if (point.x >= getWidth() || point.y >= getHeight())
        return false;
    return true;
}

template <typename T>
std::optional<Identifiable> Grid<T>::tryGetID(IntPoint2 point){
    if (!isValidPoint(point))
        return std::nullopt;
    return matrix[point.y][point.x];
}

template <typename T>
std::optional<T> Grid<T>::tryGetTile(IntPoint2 point){
    if (auto id = tryGetID(point))
        return tileset[id];
    return std::nullopt;
}

template <typename T> 
template <std::ranges::input_range R>
requires std::convertible_to<std::ranges::range_value_t<R>, T>
Grid<T>::Grid(int width, int height, R&& range) : Grid(width, height){
    tileset.reserve(range.size());
    for (const T& t : range) {
        tileset[static_cast<Identifiable>(t)] = t;
        tileIDs.push_back(static_cast<Identifiable>(t));
    }
}