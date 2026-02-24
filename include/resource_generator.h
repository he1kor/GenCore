#pragma once
#include <vector>
#include <optional>
#include "matrix.h"
#include "noise.h"

struct NoiseOctaveParam{
    double weight;
    double minimalBlob;
    double maximalBlob;
};

template <typename T>
struct ResourceMapping{
    double minimalThreshold;
    double maximalThreshold;
    T resource;
}; 

template <typename T>
class ResourceGenerator{
private:
    std::vector<NoiseOctaveParam> octaveParams = {};
    std::vector<std::pair<double, double>> resourceThresholds = {}; // first value is minimal percentile, second is maximal to create this resource;
    std::optional<std::vector<T>> resourceMappings = std::nullopt;
    IntVector2 dimension;
    Matrix<double> noiseMap;
    bool initialized = false;
public:
    void setup(
        const std::vector<NoiseOctaveParam>& octaves,
        const std::vector<std::pair<double, double>>& resourceThresholds,
        IntVector2 dimension,
        std::optional<std::reference_wrapper<Matrix<double>>> mask = std::nullopt
    );
    void setup(
        const std::vector<NoiseOctaveParam> &octaves,
        const std::vector<ResourceMapping<T>> &resources,
        IntVector2 dimension,
        std::optional<std::reference_wrapper<Matrix<double>>> mask = std::nullopt
    );

    void regenerate(std::optional<std::reference_wrapper<Matrix<double>>> mask);
    Matrix<T> generateResourcesMap();
    Matrix<bool> generateResource(size_t resourceThresholdIndex);
    std::vector<Matrix<bool>> generateResources();
private:
    void initNoise(const std::vector<NoiseOctaveParam> &octaves, IntVector2 dimensions, std::optional<std::reference_wrapper<Matrix<double>>> mask);
};


template <typename T>
inline void ResourceGenerator<T>::setup(const std::vector<NoiseOctaveParam> &octaves, const std::vector<std::pair<double, double>> &resourceThresholds, IntVector2 dimension, std::optional<std::reference_wrapper<Matrix<double>>> mask)
{
    octaveParams = octaves;
    this->dimension = dimension;
    this->resourceThresholds = resourceThresholds;
    resourceMappings = std::nullopt;
    regenerate(mask);
}

template <typename T>
inline void ResourceGenerator<T>::setup(const std::vector<NoiseOctaveParam>& octaves, const std::vector<ResourceMapping<T>>& resources, IntVector2 dimension, std::optional<std::reference_wrapper<Matrix<double>>> mask)
{

    octaveParams = octaves;
    this->dimension = dimension;
    resourceThresholds.reserve(resources.size());
    resourceMappings = std::vector<T>();
    resourceMappings->reserve(resources.size());
    for (const ResourceMapping<T>& resource : resources){
        resourceMappings->push_back(resource.resource);
        resourceThresholds.push_back({resource.minimalThreshold, resource.maximalThreshold});
    }
    regenerate(mask);
}



template <typename T>
inline Matrix<T> ResourceGenerator<T>::generateResourcesMap()
{
    if (!initialized){
        throw std::logic_error("Resource generator was not initialized!");
    }
    if (resourceMappings == std::nullopt){
        throw std::logic_error("Resource mappings was not set!");
    }
    Matrix<T> result(dimension.x, dimension.y);
    for (size_t y = 0; y < result.getHeight(); ++y){
        for (size_t x = 0; x < result.getWidth(); ++x){
            for (size_t i = 0; i < resourceThresholds.size(); ++i){
                if (noiseMap.get(x, y) >= resourceThresholds[i].first && noiseMap.get(x, y) < resourceThresholds[i].second){
                    result.set(x, y, (*resourceMappings)[i]);
                    break;
                }
            }
        }
    }
    return result;
}

template <typename T>
inline Matrix<bool> ResourceGenerator<T>::generateResource(size_t resourceThresholdIndex)
{
    if (!initialized){
        throw std::logic_error("Resource generator was not initialized!");
    }
    if (resourceThresholdIndex >= resourceThresholds.size()){
        throw std::out_of_range("Resource threshold index out of range");
    }
    
    auto& threshold = resourceThresholds[resourceThresholdIndex];
    return Matrix<double>::mapToBinary(noiseMap, [threshold](double h) { 
        return h >= threshold.first && h < threshold.second; // Полуинтервал
    });
}

template <typename T>
inline std::vector<Matrix<bool>> ResourceGenerator<T>::generateResources()
{
    if (!initialized){
        throw std::logic_error("Resource generator was not initialized!");
    }
    std::vector<Matrix<bool>> result;
    result.reserve(resourceThresholds.size());
    for (size_t i = 0; i < resourceThresholds.size(); ++i){
        result.push_back(generateResource(i));
    }
    return result;
}

template <typename T>
inline void ResourceGenerator<T>::regenerate(std::optional<std::reference_wrapper<Matrix<double>>> mask)
{
    initNoise(octaveParams, dimension, mask);
}

template <typename T>
inline void ResourceGenerator<T>::initNoise(const std::vector<NoiseOctaveParam> &octaves, IntVector2 dimensions, std::optional<std::reference_wrapper<Matrix<double>>> mask){
    std::vector<Matrix<double>> noises;
    std::vector<double> weights;
    noises.reserve(octaveParams.size());
    weights.reserve(octaveParams.size());
    for (const NoiseOctaveParam& octave : octaves){
        noises.push_back(EllipticalBlobNoise(dimensions.x, dimensions.y, octave.minimalBlob, octave.maximalBlob).generate());
        weights.push_back(octave.weight);
    }
    noiseMap = Matrix<double>::normalizedAverage(noises, weights, mask);
    initialized = true;
}