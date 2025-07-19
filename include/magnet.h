#pragma once

#include <type_traits>

#include "magnetic.h"
#include "2d.h"
#include <functional>

template <typename T>
concept IsMagneticPoint = 
    std::is_base_of_v<DoubleVector2, T> &&
    std::is_base_of_v<Magnetic, T>;

class Magnet : private DoubleVector2{
    public:
        static constexpr auto hyperbolicDistanceToForce = [](double distance) {
            return 1.0 / (distance);
        };
        static constexpr auto hyperbolicSquaredDistanceToForce = [](double distance) {
            return 1.0 / (distance * distance);
        };

        Magnet(double force);
        Magnet(double x, double y, double force);
        Magnet(DoubleVector2 point2, double force);
        void setPos(DoubleVector2 point2);

        DoubleVector2 getRepulse(DoubleVector2 object);
        DoubleVector2 getAttract(DoubleVector2 object);
        
        template <IsMagneticPoint MagneticPointT>
        DoubleVector2 getForce(MagneticPointT object);
        
        void setForceFunction(std::function<double(double)> distanceToForce);
        
    private:
        DoubleVector2 getForce(DoubleVector2 object, double force);
        static constexpr double defaultRepulseForce = 1.0;
        static constexpr double defaultAttractForce = -1.0;
        std::function<double(double)> distanceToForce = hyperbolicDistanceToForce;
        double force;
};

template <IsMagneticPoint MagneticPointT>
DoubleVector2 Magnet::getForce(MagneticPointT object){
    return getForce(static_cast<DoubleVector2>(object), object.getSuspectibility());
}