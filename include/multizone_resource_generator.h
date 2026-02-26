#pragma once

#include <unordered_map>
#include <vector>

#include "identifiable.h"
#include "resource_generator.h"
#include "matrix.h"

template <typename T>
class MultizoneResourceGenerator{
private:
    std::unordered_map<Identifiable, ResourceGenerator<T>, IDHash> zones;
    std::unordered_map<Identifiable, Matrix<double>, IDHash> masks;
    IntVector2 dimension;
    std::vector<ResourceMapping<T>> getAverageResourceMapping(IntVector2 point);

public:
    void setup(
        const std::unordered_map<Identifiable, ResourceGenerator<T>, IDHash>& zones,
        const std::unordered_map<Identifiable, Matrix<double>, IDHash>& masks
    );

    MultizoneResourceGenerator(){};
    Matrix<T> generateResourcesMap();
    double getAverageNoiseValue(IntVector2 point);
};

template <typename T>
void MultizoneResourceGenerator<T>::setup(
    const std::unordered_map<Identifiable, ResourceGenerator<T>, IDHash> &zones,
    const std::unordered_map<Identifiable, Matrix<double>, IDHash> &masks)
{
    if (zones.size() != masks.size()){
        throw std::invalid_argument(std::format("Zones size ({}) doesn't match masks size ({})", zones.size(), masks.size()));
    }
    if (zones.empty()){
        return;
    }

    dimension = zones.begin()->second.getDimension();
    for (auto& [id, resourceGenerator] : zones){
        if (resourceGenerator.getDimension() != dimension){
            throw std::invalid_argument(
                "Zone " + id.toString() + " dimension (" + 
                resourceGenerator.getDimension().toString() + 
                ") doesn't match others dimension (" + 
                dimension.toString() + ")"
            );
        }
    }
    this->zones = zones;
    this->masks = masks;
}

template <typename T>
Matrix<T> MultizoneResourceGenerator<T>::generateResourcesMap()
{
    Matrix<T> result(dimension.x, dimension.y);
    for (int y = 0; y < result.getHeight(); ++y){
        for (int x = 0; x < result.getWidth(); ++x){
            std::vector<ResourceMapping<T>> resourceMapping = getAverageResourceMapping({x, y});
            double value = getAverageNoiseValue({x, y});
            for (size_t i = 0; i < resourceMapping.size(); ++i){
                if (value >= resourceMapping.at(i).minimalThreshold && value < resourceMapping.at(i).maximalThreshold){

                    result.set(x, y, resourceMapping.at(i).resource);
                    break;
                }
            }
        }
    }
    return result;
}

template <typename T>
double MultizoneResourceGenerator<T>::getAverageNoiseValue(IntVector2 point)
{
    double result = 0;
    
    double sumWeight = 0.0;
    for (auto& [id, resourceGenerator] : zones){
        double weight = masks.at(id).get(point);
        result += weight * resourceGenerator.getNoise().get(point);
        sumWeight += weight;
    }
    return result;
}

// Requires all the zones to have the same resource mapping order, else UB.
template <typename T>
std::vector<ResourceMapping<T>> MultizoneResourceGenerator<T>::getAverageResourceMapping(IntVector2 point)
{
    std::vector<ResourceMapping<T>> result = std::vector<ResourceMapping<T>>(zones.begin()->second.getResourceMappings().size());
    
    int nonZeroWeightsCount = 0;
    double totalWeightSum = 0.0;
    
    for (auto& [id, resourceGenerator] : zones){
        const std::vector<T>& resourceMappings = resourceGenerator.getResourceMappings();
        const std::vector<std::pair<double, double>>& resourceThresholds = resourceGenerator.getResourceThresholds();


        double weight = masks.at(id).get(point);
        totalWeightSum += weight;
        
        if (std::abs(weight) > 0.000001) { // Проверка на ноль с учетом погрешности
            nonZeroWeightsCount++;
        }
        
        for (size_t i = 0; i < result.size(); i++){
            result[i].minimalThreshold += resourceThresholds[i].first * weight;
            result[i].maximalThreshold += resourceThresholds[i].second * weight;
            result[i].resource = resourceMappings[i];
        }
    }
    
    return result;
}