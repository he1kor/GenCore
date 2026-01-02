#pragma once

#include <optional>
#include "2d.h"
#include <vector>
#include <unordered_map>
#include <ranges>
#include <stdexcept>


template<typename T>
class Matrix;

template<typename U>
concept Sortable = requires(U a, U b) {
    { a < b } -> std::convertible_to<bool>;
    { a > b } -> std::convertible_to<bool>;
};

template<typename T>
concept HasHash = requires(T a) {
    { std::hash<T>{}(a) } -> std::convertible_to<std::size_t>;
};
template<typename Container, typename T>
concept MatrixContainer = 
std::ranges::range<Container> &&
requires {
    typename Container::value_type;
    requires std::same_as<
        typename Container::value_type, 
        Matrix<T>
    >;
};

template <typename Container>
concept NumericContainer = 
    std::ranges::range<Container> &&
    std::is_arithmetic_v<typename Container::value_type>;

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

        template <typename Container>
        requires MatrixContainer<Container, T>
        static Matrix<T> average(Container& container);

        template <typename MatrixContainerType, typename WeightContainerType>
        requires MatrixContainer<MatrixContainerType, T> &&
                NumericContainer<WeightContainerType>
        static Matrix<T> average(
            const MatrixContainerType& matrices,
            const WeightContainerType& weights
        );

        template <typename Container>
        requires MatrixContainer<Container, T>
        static Matrix<T> normalizedAverage(Container& container);

        template <typename MatrixContainerType, typename WeightContainerType>
        requires MatrixContainer<MatrixContainerType, T> &&
                NumericContainer<WeightContainerType>
        static Matrix<T> normalizedAverage(
            const MatrixContainerType& matrices,
            const WeightContainerType& weights
        );

    
    template<typename U = T>
    requires Sortable<U> && HasHash<U>
    void normalizeToPercentiles();

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

                template<typename U = T>
                U& operator*() requires(!std::same_as<U, bool>) {
                    return matrix.matrix[y][x];
                }
                
                bool operator*() requires(std::same_as<T, bool>) {
                    return matrix.matrix[y][x];
                }
                
                template<typename U = T>
                const U& operator*() const requires(!std::same_as<U, bool>) {
                    return matrix.matrix[y][x];
                }
                
                bool operator*() const requires(std::same_as<T, bool>) {
                    return matrix.matrix[y][x];
                }
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

template <typename T>
template <typename MatrixContainerType, typename WeightContainerType>
requires MatrixContainer<MatrixContainerType, T> &&
        NumericContainer<WeightContainerType>
inline Matrix<T> Matrix<T>::average(
    const MatrixContainerType& matrices,
    const WeightContainerType& weights
){
    if (matrices.empty()){
        throw std::invalid_argument("Matrix:average: matrices can't be empty");
    }
    if (std::ranges::size(matrices) != std::ranges::size(weights)){
        throw std::invalid_argument("Matrix:average: matrices and weights sizes should be equal");
    }
    size_t width = matrices.begin()->getWidth();
    size_t height = matrices.begin()->getHeight();
    Matrix<T> result(width, height);
    auto weightsIt = weights.begin();
    double weightSum = 0;
    for (const Matrix<T>& matrix : matrices) {
        double weight = *weightsIt;
        weightSum += weight;
        if (matrix.getWidth() != width || matrix.getHeight() != height){
            throw std::invalid_argument("Matrix:average: each matrix should have the same size");
        }
        for (size_t y = 0; y < height; ++y){
            for (size_t x = 0; x < width; ++x){
                result.set(x, y, result.get(x, y) + (matrix.get(x, y) * weight));
            }
        }
        weightsIt++;
    }
    for (T& pixel : result){
        pixel /= weightSum;
    }
    return result;
}

template <typename T>
template <typename Container>
requires MatrixContainer<Container, T>
inline Matrix<T> Matrix<T>::average(Container &matrices){
    std::vector<double> weights = std::vector<double>(std::ranges::size(matrices), 1.0);
    return average(matrices, weights);
}

template <typename T>
template <typename MatrixContainerType, typename WeightContainerType>
requires MatrixContainer<MatrixContainerType, T> &&
        NumericContainer<WeightContainerType>
inline Matrix<T> Matrix<T>::normalizedAverage(
    const MatrixContainerType& matrices,
    const WeightContainerType& weights
){
    Matrix<T> result = average(matrices, weights);
    result.normalizeToPercentiles();
    return result;
}

template <typename T>
template <typename Container>
requires MatrixContainer<Container, T>
inline Matrix<T> Matrix<T>::normalizedAverage(Container &matrices){
    Matrix<T> result = average(matrices);
    result.normalizeToPercentiles();
    return result;
}

template <typename T>
template <typename U>
    requires Sortable<U> && HasHash<U>
void Matrix<T>::normalizeToPercentiles()
{
    std::unordered_map<U, std::vector<std::pair<int, int>>> valuePositions;
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            valuePositions[matrix[y][x]].emplace_back(x, y);
        }
    }
    
    std::vector<U> uniqueValues;
    uniqueValues.reserve(valuePositions.size());
    for (const auto& [value, _] : valuePositions) {
        uniqueValues.push_back(value);
    }
    
    std::sort(uniqueValues.begin(), uniqueValues.end());
    int cumulativeCount = 0;
    int total = width * height;
    
    for (const auto& value : uniqueValues) {
        const auto& positions = valuePositions[value];
        int count = positions.size();
        
        double percentile = (cumulativeCount + count / 2.0) / total;
        
        T percentileValue;
        if constexpr (std::is_floating_point_v<T>) {
            percentileValue = static_cast<T>(percentile);
        } else if constexpr (std::is_integral_v<T>) {
            percentileValue = static_cast<T>(percentile * std::numeric_limits<T>::max());
        } else {
            percentileValue = T(percentile);
        }
        
        for (const auto& [x, y] : positions) {
            matrix[y][x] = percentileValue;
        }
        
        cumulativeCount += count;
    }
}
