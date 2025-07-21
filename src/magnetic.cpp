#include "magnetic.h"

double Magnetic::getSuspectibility() const{
    return susceptibility;
}

bool Magnetic::isAttractive() const{
    return getSuspectibility() > 0;
}

bool Magnetic::isRepulsive() const{
    return getSuspectibility() < 0;
}

bool Magnetic::isNotMagnetic() const{
    return getSuspectibility() == 0;
}

bool Magnetic::isMagnetic() const{
    return !isNotMagnetic();
}

void Magnetic::reversePole(){
    setSuspectibility(-getSuspectibility());
}

Magnetic::Magnetic(){}

void Magnetic::setSuspectibility(double val){
    this->susceptibility = val;
}
