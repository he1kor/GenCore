#pragma once

#include <type_traits>

#include "magnetic.h"
#include "2d.h"
#include <functional>
#include <cmath>

template <typename T>
concept IsMagneticPoint =
    std::is_base_of_v<Magnetic, T>;


class Magnet : public SafeDoubleVector2{
    public:
        static constexpr auto linearDistanceToForce = [](double distance) {
            constexpr double maxDistance = 5.0;
            distance = std::min(maxDistance, distance);
            return 1;
        };
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

        DoubleVector2 calculateForce(const DoubleVector2& object) const;
        
        template <IsMagneticPoint MagneticPointT>
        DoubleVector2 calculateForce(const DoubleVector2& object, const MagneticPointT& magnetic) const;

        double getForce() const{return force;};
        
        void setForceFunction(std::function<double(double)> distanceToForce);
    protected:
        Magnet() : force(0){};
    private:
        DoubleVector2 calculateForce(const DoubleVector2& object, double force) const;
        static constexpr double defaultRepulseForce = 1.0;
        static constexpr double defaultAttractForce = -1.0;
        std::function<double(double)> distanceToForce = linearDistanceToForce;
        double force;
};

template <IsMagneticPoint MagneticPointT>
DoubleVector2 Magnet::calculateForce(const DoubleVector2& object, const MagneticPointT& magnetic) const{
    return calculateForce(static_cast<DoubleVector2>(object), magnetic.getSuspectibility());
}