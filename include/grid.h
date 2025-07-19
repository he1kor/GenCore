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
        Grid(const std::vector<std::vector<T>>& matrix);

        template<std::ranges::input_range R>
        requires std::convertible_to<std::ranges::range_value_t<R>, T>
        Grid(int width, int height, R&& range);

        void setTile(int x, int y, Identifiable value);
        void setTile(IntVector2 point, Identifiable value);
        Identifiable getTileID(int x, int y) const;
        const std::vector<Identifiable>& getTileIDs() const;
        T getTile(int x, int y) const;
        int getWidth() const;
        int getHeight() const;
        
        bool isEmpty(int x, int y);
        bool isEmpty(IntVector2 point);

        std::vector<T> applyToDoublePoints(DoubleVector2 size);

        bool isValidPoint(IntVector2 point2);

        std::optional<Identifiable>tryGetID(IntVector2 point2);
        std::optional<T>tryGetTile(IntVector2 point2);

    private:
        int width = -1;
        int height = -1;
        std::vector<std::vector<Identifiable>> matrix;
        std::unordered_map<Identifiable, T, IDHash> tileset;
        std::vector<Identifiable> tileIDs;
};




template <typename T> 
Grid<T>::Grid(int width, int height) : width(width), height(height){
    matrix = std::vector<std::vector<Identifiable>>(height, std::vector<Identifiable>(width));
}

template <typename T>
Grid<T>::Grid(const std::vector<std::vector<T>>& matrix){
    if (matrix.empty()){
        this->width = 0;
        this->height = 0;
        return;
    }
    this->matrix = std::vector<std::vector<T>>(matrix.size());
    size_t width = matrix[0].size();
    for (int y = 0; y < matrix.size(); y++){
        this->matrix[y].reserve(width);
        for (int x = 0 ; x < width; x++){
            auto const& value = matrix.at[y].at[x];
            if (!tileset.count(value)){
                tileset[static_cast<Identifiable>(value)] = value;
                tileIDs.push_back(static_cast<Identifiable>(value));
            }
            this->matrix[y].push_back(matrix[y][x]);
        }
    }
}

template <typename T>
void Grid<T>::setTile(int x, int y, Identifiable value){
    matrix.at(y).at(x) = value;
}

template <typename T>
void Grid<T>::setTile(IntVector2 point, Identifiable value){
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
bool Grid<T>::isEmpty(IntVector2 point){
    return isEmpty(point.x, point.y);
}

template <typename T>
std::vector<T> Grid<T>::applyToDoublePoints(DoubleVector2 size){
    static_assert(std::is_base_of_v<DoubleVector2, T>, "T must inherit from DoubleVector2");
    std::vector<T> result;
    for (int y = 0; y < getHeight(); y++){
        for (int x = 0; x < getWidth(); x++){
            if (!isEmpty(x, y)){
                DoubleVector2& point = getTile(x, y);
                double yPercentage = static_cast<double>(y) / (getHeight() - 1);
                double xPercentage = static_cast<double>(x) / (getWidth() - 1);
                point.x = size.x * xPercentage;
                point.y = size.y * yPercentage;
                result.push_back(point);
            }
        }
    }
    return result;
}

template <typename T>
bool Grid<T>::isValidPoint(IntVector2 point){
    if (point.x < 0 || point.y < 0)
        return false;
    if (point.x >= getWidth() || point.y >= getHeight())
        return false;
    return true;
}

template <typename T>
std::optional<Identifiable> Grid<T>::tryGetID(IntVector2 point){
    if (!isValidPoint(point))
        return std::nullopt;
    return matrix[point.y][point.x];
}

template <typename T>
std::optional<T> Grid<T>::tryGetTile(IntVector2 point){
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