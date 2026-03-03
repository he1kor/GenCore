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
    double minimalThreshold = 0.0;
    double maximalThreshold = 0.0;
    T resource;
}; 

template <typename T>
class ResourceGenerator{
private:
    std::vector<std::pair<double, double>> resourceThresholds = {}; // first value is minimal percentile, second is maximal to create this resource;
    std::optional<std::vector<T>> resourceMappings = std::nullopt;
    IntVector2 dimension;
    Matrix<double> noiseMap;
    bool initialized = false;
public:
    void setup(
        const std::vector<Matrix<double>>& octaves,
        const std::vector<double>& octaveWeights,
        const std::vector<std::pair<double, double>>& resourceThresholds,
        IntVector2 dimension,
        std::optional<std::reference_wrapper<Matrix<double>>> mask = std::nullopt
    );
    void setup(
        const std::vector<Matrix<double>> &octaves,
        const std::vector<double>& octaveWeights,
        const std::vector<ResourceMapping<T>> &resources,
        IntVector2 dimension,
        std::optional<std::reference_wrapper<Matrix<double>>> mask = std::nullopt
    );

    ResourceGenerator(){};

    Matrix<T> generateResourcesMap();
    Matrix<bool> generateResource(size_t resourceThresholdIndex);
    std::vector<Matrix<bool>> generateResources();
    void regenerate(const std::vector<Matrix<double>> &octaves, const std::vector<double>& octaveWeights, std::optional<std::reference_wrapper<Matrix<double>>> mask);
    const Matrix<double> &getNoise() const { return noiseMap; };
    IntVector2 getDimension() const {return dimension;};
    const std::vector<T>& getResourceMappings() const {return resourceMappings.value();}
    const std::vector<std::pair<double, double>>& getResourceThresholds() const {return resourceThresholds;}

private:
    void initNoise(const std::vector<Matrix<double>> &octaves, const std::vector<double>& octaveWeights, IntVector2 dimensions, std::optional<std::reference_wrapper<Matrix<double>>> mask);
};


template <typename T>
inline void ResourceGenerator<T>::setup(const std::vector<Matrix<double>> &octaves, const std::vector<double>& octaveWeights, const std::vector<std::pair<double, double>> &resourceThresholds, IntVector2 dimension, std::optional<std::reference_wrapper<Matrix<double>>> mask)
{
    this->dimension = dimension;
    this->resourceThresholds = resourceThresholds;
    resourceMappings = std::nullopt;
    regenerate(octaves, mask);
}

template <typename T>
inline void ResourceGenerator<T>::setup(const std::vector<Matrix<double>>& octaves, const std::vector<double>& octaveWeights, const std::vector<ResourceMapping<T>>& resources, IntVector2 dimension, std::optional<std::reference_wrapper<Matrix<double>>> mask)
{
    this->dimension = dimension;
    resourceThresholds.reserve(resources.size());
    resourceMappings = std::vector<T>();
    resourceMappings->reserve(resources.size());
    for (const ResourceMapping<T>& resource : resources){
        resourceMappings->push_back(resource.resource);
        resourceThresholds.push_back({resource.minimalThreshold, resource.maximalThreshold});
    }
    regenerate(octaves, octaveWeights, mask);
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
        return h >= threshold.first && h < threshold.second;
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
inline void ResourceGenerator<T>::regenerate(const std::vector<Matrix<double>> &octaves, const std::vector<double>& octaveWeights, std::optional<std::reference_wrapper<Matrix<double>>> mask)
{
    initNoise(octaves, octaveWeights, dimension, mask);
}

template <typename T>
inline void ResourceGenerator<T>::initNoise(const std::vector<Matrix<double>>& octaves, const std::vector<double>& octaveWeights, IntVector2 dimensions, std::optional<std::reference_wrapper<Matrix<double>>> mask){
    std::vector<Matrix<double>> noises;
    noises.reserve(octaves.size());
    for (const Matrix<double>& octave : octaves){
        noises.push_back(octave);
    }
    noiseMap = Matrix<double>::normalizedAverage(noises, octaveWeights, mask);
    initialized = true;
}