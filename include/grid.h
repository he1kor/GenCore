#pragma once

#include <vector>
#include <unordered_map>
#include <optional>

#include <iostream>

#include "2d.h"
#include "identifiable.h"


#include <exception>
#include <string>

class NoTileException : public std::exception {
private:
    std::string message;
    
public:
    explicit NoTileException(const std::string& msg) : message(msg) {}
    
    const char* what() const noexcept override {
        return message.c_str();
    }
};

template<typename T>
class Grid{
    static_assert(std::is_base_of_v<Identifiable, T>, "T must inherit from Identifiable");
    public:
        Grid(int width, int height);
        Grid(const std::vector<std::vector<T>>& matrix);

        template<std::ranges::input_range R>
        requires std::convertible_to<std::ranges::range_value_t<R>, T>
        Grid(int width, int height, R&& range);

        void checkTilesetValidness(Identifiable id) const;
        void setTile(int x, int y, Identifiable value);
        void setTile(IntVector2 point, Identifiable value);
        Identifiable getTileID(int x, int y) const;
        const std::vector<Identifiable>& getTileIDs() const;
        const T& getTile(int x, int y) const;
        const T& getTile(Identifiable id) const;
        int getWidth() const;
        int getHeight() const;
        
        bool isEmpty(int x, int y);
        bool isEmpty(IntVector2 point);

        std::vector<T> applyToDoublePoints(DoubleVector2 size);

        bool isValidPoint(IntVector2 point2);

        std::optional<Identifiable>tryGetID(IntVector2 point2);
        std::optional<T>tryGetTile(IntVector2 point2);

        class Iterator{
            private:
                const Grid& grid;
                int y;
                int x;
            public:
                int getX() const{return x;};
                int getY() const{return y;};
                void move(int x, int y){this->y += y; this->x += x;};
                Iterator(const Grid& grid, int x, int y) : grid(grid), x(x), y(y){}
                Identifiable operator*() const{return grid.getTileID(x, y);}
                Iterator& operator++();
                Iterator& operator--();
                bool operator==(const Iterator& other) const;
                bool operator!=(const Iterator& other) const{return !(*this == other);}
        };
        Iterator begin() { return Iterator(*this, 0, 0); }
        Iterator end() { return Iterator(*this, 0, getHeight()); }

    private:
        int width = -1;
        int height = -1;
        std::vector<std::vector<Identifiable>> matrix;
        std::unordered_map<Identifiable, T, IDHash> tileset;
        std::vector<Identifiable> tileIDs;
};

template <typename T>
Grid<T>::Iterator& Grid<T>::Iterator::operator++(){
    x++;
    if (x >= grid.getWidth()){
        x = 0;
        y++;
    }
    return *this;
}
template <typename T>
Grid<T>::Iterator& Grid<T>::Iterator::operator--(){
    x--;
    if (x < 0){
        x = grid.getWidth()-1;
        y--;
    }
    return *this;
}

template <typename T>
bool Grid<T>::Iterator::operator==(const Iterator &other) const{
    return 
        this->grid.matrix.data() == other.grid.matrix.data() &&
        this->x == other.x &&
        this->y == other.y;
}

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
    size_t height = matrix.size();
    size_t width = matrix[0].size();
    this->matrix = std::vector<std::vector<Identifiable>>(height);
    for (int y = 0; y < height; y++){
        this->matrix[y].reserve(width);
        for (int x = 0 ; x < width; x++){
            const T& value = matrix.at(y).at(x);
            if (!tileset.count(value)){
                tileset[static_cast<Identifiable>(value)] = value;
                tileIDs.push_back(static_cast<Identifiable>(value));
            }
            this->matrix[y].push_back(matrix[y][x]);
        }
    }
    this->width = width;
    this->height = height;
}

template <typename T>
void Grid<T>::checkTilesetValidness(Identifiable id) const
{
    if (!tileset.count(id)){
        if (id == Identifiable::nullID)
            throw NoTileException("The provided ID is nullID\n");
        else
            throw NoTileException("bad tileset - No tile is related to the given ID\n");
    }
}

template <typename T>
void Grid<T>::setTile(int x, int y, Identifiable value)
{
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
const T& Grid<T>::getTile(int x, int y) const{
    Identifiable id = getTileID(x, y);
    return getTile(id);
}

template <typename T>
const T& Grid<T>::getTile(Identifiable id) const{
    checkTilesetValidness(id);
    return tileset.at(id);
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
    static_assert(std::is_base_of_v<SafeDoubleVector2, T>, "T must inherit from SafeDoubleVector2");
    std::vector<T> result;
    for (int y = 0; y < getHeight(); y++){
        for (int x = 0; x < getWidth(); x++){
            if (!isEmpty(x, y)){
                T point = getTile(x, y);
                double yPercentage = static_cast<double>(y) / (getHeight() - 1);
                double xPercentage = static_cast<double>(x) / (getWidth() - 1);
                point.setX(size.x * xPercentage);
                point.setY(size.y * yPercentage);
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
    if (auto id = tryGetID(point)){
        return getTile(id);
    }
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