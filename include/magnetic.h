#pragma once
#include <iostream>

class Magnetic{
    public:
        Magnetic();
        constexpr Magnetic(double val) : susceptibility(val){}
        void setSuspectibility(double val);
        double getSuspectibility() const;
        bool isAttractive() const;
        bool isRepulsive() const;
        bool isNotMagnetic() const;
        bool isMagnetic() const;
        void reversePole();
    private:
        double susceptibility = 0;
};