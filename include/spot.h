#pragma once
#include <utility>
#include <memory>
#include "identifiable.h"

struct Point2 {
    double x = 0;
    double y = 0;
};

template<typename T>
class Spot {
    static_assert(std::is_base_of_v<Identifiable, T>, "T must inherit from Identifiable");
    public:
        Spot();
        Spot(double x, double y);
        Spot(double x, double y, T t);
        void setX(double x);
        void setY(double y);
        void changeX(double dx);
        void changeY(double dy);
        Point2 getCoords();
        double getX() const;
        double getY() const;
        const T& getValue() const;
        Identifiable getIdentifiable() const;
        
    private:
        double x;
        double y;
        T t;
};

template <typename T>
Spot<T>::Spot() : x(-1), y(-1), t(T()) {}

template <typename T>
Spot<T>::Spot(double x, double y) : x(x), y(y), t(T()) {}

template <typename T>
Spot<T>::Spot(double x, double y, T t) : x(x), y(y), t(t) {}

template <typename T>
void Spot<T>::setX(double x) {
    this->x = x;
}

template <typename T>
void Spot<T>::setY(double y) {
    this->y = y;
}

template <typename T>
void Spot<T>::changeX(double dx) {
    x += dx;
}

template <typename T>
void Spot<T>::changeY(double dy) {
    y += dy;
}

template <typename T>
double Spot<T>::getX() const {
    return x;
}

template <typename T>
double Spot<T>::getY() const {
    return y;
}

template <typename T>
const T& Spot<T>::getValue() const {
    return t;
}

template <typename T>
Identifiable Spot<T>::getIdentifiable() const {
    return static_cast<Identifiable>(t);
}

template <typename T>
Point2 Spot<T>::getCoords() {
    return {x, y};
}