#pragma once

#include <map>
#include <numeric>
#include <unordered_map>
#include <vector>

#include "identifiable.h"
#include "resource_generator.h"
#include "matrix.h"

template <typename T>
class MultizoneResourceGenerator {
private:
    std::map<Identifiable, ResourceGenerator<T>> zones;
    std::unordered_map<Identifiable, Matrix<double>, IDHash> masks;
    std::map<Identifiable, std::map<T, double>> materialShares;
    IntVector2 dimension;
    std::vector<ResourceMapping<T>> getAverageResourceMapping(IntVector2 point);

public:
    void setup(
        const std::unordered_map<Identifiable, ResourceGenerator<T>, IDHash>& zones,
        const std::unordered_map<Identifiable, Matrix<double>, IDHash>& masks
    );

    Matrix<T> generateResourcesMap(T emptyResource);
    double getAverageNoiseValue(IntVector2 point);
};

template <typename T>
void MultizoneResourceGenerator<T>::setup(
    const std::unordered_map<Identifiable, ResourceGenerator<T>, IDHash>& zones,
    const std::unordered_map<Identifiable, Matrix<double>, IDHash>& masks)
{
    if (zones.empty() || zones.size() != masks.size()) {
        throw std::invalid_argument("Resource zones and masks must be nonempty and have matching IDs");
    }
    dimension = zones.begin()->second.getDimension();
    for (const auto& [id, generator] : zones) {
        auto mask = masks.find(id);
        if (mask == masks.end() || mask->second.getWidth() != dimension.x || mask->second.getHeight() != dimension.y
            || generator.getDimension() != dimension) {
            throw std::invalid_argument("Resource zones and their masks must have matching IDs and dimensions");
        }
    }
    this->zones = std::map<Identifiable, ResourceGenerator<T>>(zones.begin(), zones.end());
    this->masks = masks;
    materialShares.clear();
    for (const auto& [id, generator] : this->zones) {
        const auto& materials = generator.getResourceMappings();
        const auto& thresholds = generator.getResourceThresholds();
        std::vector<size_t> order(materials.size());
        std::iota(order.begin(), order.end(), 0);
        std::sort(order.begin(), order.end(), [&](size_t a, size_t b) { return thresholds[a] < thresholds[b]; });
        for (size_t i : order) {
            const double share = thresholds[i].second - thresholds[i].first;
            if (share > 0.0) materialShares[id][materials[i]] += share;
        }
    }
}

template <typename T>
Matrix<T> MultizoneResourceGenerator<T>::generateResourcesMap(T emptyResource) {
    if (zones.empty()) throw std::logic_error("Resource generator was not set up");
    Matrix<T> result(dimension.x, dimension.y, emptyResource);
    for (int y = 0; y < result.getHeight(); ++y) {
        for (int x = 0; x < result.getWidth(); ++x) {
            const auto mapping = getAverageResourceMapping({x, y});
            if (mapping.empty()) continue; // Walls and other cells outside all zone masks.
            const double value = getAverageNoiseValue({x, y});
            bool assigned = false;
            for (const auto& resource : mapping) {
                if (value >= resource.minimalThreshold && value < resource.maximalThreshold) {
                    result.set(x, y, resource.resource);
                    assigned = true;
                    break;
                }
            }
            if (!assigned) throw std::domain_error("Noise value is not covered by the blended resource thresholds");
        }
    }
    return result;
}

template <typename T>
double MultizoneResourceGenerator<T>::getAverageNoiseValue(IntVector2 point) {
    double value = 0.0;
    double totalWeight = 0.0;
    for (const auto& [id, generator] : zones) {
        const double weight = masks.at(id).get(point);
        if (weight <= 0.0) continue;
        value += generator.getNoise().get(point) * weight;
        totalWeight += weight;
    }
    return totalWeight > 0.0 ? value / totalWeight : 0.0;
}

template <typename T>
std::vector<ResourceMapping<T>> MultizoneResourceGenerator<T>::getAverageResourceMapping(IntVector2 point) {
    const ResourceGenerator<T>* onlyZone = nullptr;
    size_t activeZones = 0;
    for (const auto& [id, generator] : zones) {
        if (masks.at(id).get(point) > 0.0) {
            onlyZone = &generator;
            ++activeZones;
        }
    }
    if (activeZones == 1) {
        std::vector<ResourceMapping<T>> result;
        const auto& materials = onlyZone->getResourceMappings();
        const auto& thresholds = onlyZone->getResourceThresholds();
        for (size_t i = 0; i < materials.size(); ++i) {
            result.push_back({thresholds[i].first, thresholds[i].second, materials[i]});
        }
        return result;
    }
    // Blend percentile shares by material identity. Missing materials have zero share.
    // Material identity supplies a stable order, independent of JSON and hash-map order.
    std::map<T, double> shares;
    double totalShare = 0.0;
    for (const auto& [id, generator] : zones) {
        const double weight = masks.at(id).get(point);
        if (weight <= 0.0) continue;
        for (const auto& [material, share] : materialShares.at(id)) {
            shares[material] += share * weight;
        }
    }
    for (const auto& [material, share] : shares) totalShare += share;
    std::vector<ResourceMapping<T>> result;
    double start = 0.0;
    for (const auto& [material, share] : shares) {
        const double end = start + share / totalShare;
        result.push_back({start, end, material});
        start = end;
    }
    if (!result.empty()) result.back().maximalThreshold = 1.0;
    return result;
}
