#pragma once

struct DoubleVector2 {
    DoubleVector2(double x = 0, double y = 0) : x(x), y(y) {}
    double x = 0;
    double y = 0;
    DoubleVector2 operator*(double scalar) const;
};

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
};