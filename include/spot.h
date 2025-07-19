#pragma once
#include <utility>
#include <memory>
#include "identifiable.h"
#include "2d.h"
#include "magnet.h"

template<typename T>
class Spot : private DoubleVector2 {
    static_assert(std::is_base_of_v<Identifiable, T>, "T must inherit from Identifiable");
    public:
        Spot();
        Spot(double x, double y);
        Spot(double x, double y, T t);
        void setX(double x);
        void setY(double y);
        void changeX(double dx);
        void changeY(double dy);
        DoubleVector2 getCoords();
        double getX() const;
        double getY() const;
        const T& getValue() const;
        Identifiable getIdentifiable() const;
        
    private:
        T t;
};

template <typename T>
Spot<T>::Spot() : DoubleVector2{-1, -1}, t(T()) {}

template <typename T>
Spot<T>::Spot(double x, double y) : DoubleVector2{x, y}, t(T()) {}

template <typename T>
Spot<T>::Spot(double x, double y, T t) : DoubleVector2{x, y}, t(t) {}

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
DoubleVector2 Spot<T>::getCoords() {
    return {x, y};
}