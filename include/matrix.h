#pragma once

#include <optional>
#include "2d.h"

#include <stdexcept>

template <typename T>
class Matrix{
    public:
        Matrix(int width, int height);
        Matrix(const std::vector<std::vector<T>>& matrix);
        Matrix(std::initializer_list<std::initializer_list<T>> init);

        void set(int x, int y, const T& value);
        const T get(int x, int y) const;
        int getWidth() const;
        int getHeight() const;

        Matrix(const Matrix& other);

        bool isValidPoint(IntVector2 point2);

        std::optional<const T&> tryGet(IntVector2 point2);

        class Iterator{
            private:
                Matrix& matrix;
                int y;
                int x;
            public:
                int getX() const{return x;};
                int getY() const{return y;};
                void move(int x, int y){this->y += y; this->x += x;};
                Iterator(Matrix& matrix, int x, int y) : matrix(matrix), x(x), y(y){}
                auto operator*() {return matrix.matrix[y][x];}
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
        std::vector<std::vector<T>> matrix;
};

template <typename T>
Matrix<T>::Iterator& Matrix<T>::Iterator::operator++(){
    x++;
    if (x >= matrix.getWidth()){
        x = 0;
        y++;
    }
    return *this;
}
template <typename T>
Matrix<T>::Iterator& Matrix<T>::Iterator::operator--(){
    x--;
    if (x < 0){
        x = matrix.getWidth()-1;
        y--;
    }
    return *this;
}

template <typename T>
bool Matrix<T>::Iterator::operator==(const Iterator &other) const{
    return 
        this->matrix.matrix.data() == other.matrix.matrix.data() &&
        this->x == other.x &&
        this->y == other.y;
}


template <typename T> 
Matrix<T>::Matrix(int width, int height) : width(width), height(height){
    matrix = std::vector<std::vector<T>>(height, std::vector<T>(width));
}

template <typename T>
Matrix<T>::Matrix(const std::vector<std::vector<T>> &matrix){
    if (matrix.empty()){
        this->width = 0;
        this->height = 0;
        return;
    }
    size_t height = matrix.size();
    size_t width = matrix[0].size();
    this->matrix = std::vector<std::vector<T>>(height);
    for (int y = 0; y < height; y++){
        this->matrix[y].reserve(width);
        for (int x = 0; x < width; x++){
            this->matrix[y].push_back(
                matrix.at(y).at(x)
            );
        }
    }
    this->width = width;
    this->height = height;
}

template <typename T>
Matrix<T>::Matrix(std::initializer_list<std::initializer_list<T>> init){
    if (init.size() == 0) {
        width = 0;
        height = 0;
        return;
    }
    height = init.size();
    width = init.begin()->size();
    matrix.reserve(height);
    for (const auto& row : init) {
        if (row.size() != width) {
            throw std::invalid_argument("All rows must have the same size");
        }
        matrix.emplace_back(row);
    }
}

template <typename T>
void Matrix<T>::set(int x, int y, const T &value){
    matrix.at(y).at(x) = value;
}

template <typename T>
const T Matrix<T>::get(int x, int y) const{
    return matrix.at(y).at(x);
}

template <typename T>
Matrix<T>::Matrix(const Matrix& other){
    width = other.width;
    height = other.height;
    matrix = other.matrix;
}

template <typename T>
bool Matrix<T>::isValidPoint(IntVector2 point){
    if (point.x < 0 || point.y < 0)
        return false;
    if (point.x >= getWidth() || point.y >= getHeight())
        return false;
    return true;
}

template <typename T>
std::optional<const T &> Matrix<T>::tryGet(IntVector2 point2){
    if (!isValidPoint(point2))
        return std::nullopt;
    return matrix[point2.y][point2.x];
}

template <typename T>
int Matrix<T>::getWidth() const{
    return width;
}

template <typename T>
int Matrix<T>::getHeight() const{
    return height;
}