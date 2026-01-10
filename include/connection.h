#pragma once

#include "identifiable.h"
#include <unordered_map>
#include <memory>
#include <type_traits>

class Blendable{
public:
    int distance = 0;
};

class ResourceGuarantor {
public:
    int areaGuaranteed = 0;
    double bonusValue = 0.0;
};

class GapMaker{
    
};

class BasicConnection : Identifiable {
    
}; 