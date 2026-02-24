#pragma once

#include "matrix.h"

class Noise{
private:
    int width;
    int height;
protected:
    Noise(int width, int height) : width(width), height(height){};
public:
    virtual Matrix<double> generate() = 0;
    int getWidth() const {return width;}
    virtual ~Noise() = default;
    int getHeight() const {return height;}
};

class EllipticalBlobNoise : public Noise{
private:
    double minRadius;
    double maxRadius;

public: 
    EllipticalBlobNoise(int width, int height, double minRadius, double maxRadius) :
        Noise(width, height),
        minRadius(minRadius),
        maxRadius(maxRadius){
        if (minRadius <= 0 || maxRadius <= 0) {
            throw std::invalid_argument("Radius must be positive");
        }
        if (minRadius > maxRadius) {
            throw std::invalid_argument("minRadius cannot be greater than maxRadius");
        }
    };
    Matrix<double> generate() override;
    
    double getMinRadius() const {return minRadius;}
    double getMaxRadius() const {return maxRadius;}

    void setRadiusRange(double minRadius, double maxRadius){
        this->minRadius = minRadius;
        this->maxRadius = maxRadius;
    };
};