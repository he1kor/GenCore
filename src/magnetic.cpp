#include "magnetic.h"

double Magnetic::getSusceptibility() const{
    return susceptibility;
}

bool Magnetic::isAttractive() const{
    return getSusceptibility() > 0;
}

bool Magnetic::isRepulsive() const{
    return getSusceptibility() < 0;
}

bool Magnetic::isNotMagnetic() const{
    return getSusceptibility() == 0;
}

bool Magnetic::isMagnetic() const{
    return !isNotMagnetic();
}

void Magnetic::reversePole(){
    setSusceptibility(-getSusceptibility());
}

Magnetic::Magnetic(){}

void Magnetic::setSusceptibility(double val){
    this->susceptibility = val;
}
