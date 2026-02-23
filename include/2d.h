#pragma once

#include <iostream>

struct DoubleVector2 {
    DoubleVector2(double x = 0, double y = 0) : x(x), y(y) {}
    double x = 0;
    double y = 0;
    DoubleVector2 operator*(double scalar) const;
    DoubleVector2 operator+(const DoubleVector2& other) const;
    DoubleVector2 operator-(const DoubleVector2& other) const;
};

namespace tiles{
    enum class Orientation{
        vertical,
        horizontal
    };

    enum class Direction{
        up,
        right,
        down,
        left,
    };
    void turnRight(Direction& direction);
    void turnLeft(Direction& direction);
    void turnBackward(Direction &direction);
    Orientation toOrientation(Direction direction);
}

struct SafeDoubleVector2 : protected DoubleVector2{
    SafeDoubleVector2(double x = 0, double y = 0) : DoubleVector2{x, y}{};
    SafeDoubleVector2(const DoubleVector2& another) : DoubleVector2{another.x, another.y}{};
    virtual double getX() const{return x;};
    virtual double getY() const{return y;};
    virtual void setX(double x){this->x = x;};
    virtual void setY(double y){this->y = y;};
};

struct IntVector2 {
    int x = 0;
    int y = 0;
    IntVector2 operator*(int scalar) const;
    IntVector2 operator+(const IntVector2& other) const;
    IntVector2 operator-(const IntVector2& other) const;
    bool operator==(const IntVector2& other) const;

    std::string toString() const{
        return "{" + std::to_string(x) + "; " + std::to_string(y) + "}";
    }
};

std::ostream& operator<<(std::ostream& os, const IntVector2& vec);
std::ostream& operator<<(std::ostream& os, tiles::Direction direction);