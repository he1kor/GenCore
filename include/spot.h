#pragma once
#include <utility>
#include <memory>
#include "identifiable.h"
#include "2d.h"
#include "magnet.h"

template<typename T>
class Spot : public SafeDoubleVector2 {
    static_assert(std::is_base_of_v<Identifiable, T>, "T must inherit from Identifiable");
    public:
        Spot();
        Spot(double x, double y);
        Spot(double x, double y, T t);
        void changeX(double dx);
        void changeY(double dy);
        DoubleVector2 getCoords() const;
        const T& getValue() const;
        Identifiable getIdentifiable() const;
        
    private:
        T t;
};

template <typename T>
Spot<T>::Spot() : SafeDoubleVector2{-1, -1}, t(T()) {}

template <typename T>
Spot<T>::Spot(double x, double y) : SafeDoubleVector2{x, y}, t(T()) {}

template <typename T>
Spot<T>::Spot(double x, double y, T t) : SafeDoubleVector2{x, y}, t(t) {}

template <typename T>
void Spot<T>::changeX(double dx) {
    x += dx;
}

template <typename T>
void Spot<T>::changeY(double dy) {
    y += dy;
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
DoubleVector2 Spot<T>::getCoords() const {
    return {x, y};
}