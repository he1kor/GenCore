#include "random_generator.h"

#include <stdexcept>
#include <limits>

RandomGenerator::RandomGenerator(){
    generateSeed();
}

RandomGenerator &RandomGenerator::instance(){
    static RandomGenerator randomGenerator;
    return randomGenerator;
}

bool RandomGenerator::chanceOccurred(double probability){
    if (probability < 0.0 || probability > 1.0) {
        throw std::invalid_argument("Probability must be between 0.0 and 1.0");
    }
    
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    
    return dist(generator) < probability;
}

size_t RandomGenerator::takenIndex(const std::vector<uint64_t>& weightedItems){
    uint64_t upper = 0;
    bool overflow = false;
    for (uint64_t weight : weightedItems) {
        if (weight > std::numeric_limits<uint64_t>::max() - upper) { overflow = true; break; }
        upper += weight;
    }
    if (overflow) {
        long double total = 0;
        for (uint64_t weight : weightedItems) total += weight;
        const long double draw = std::uniform_real_distribution<long double>(0, total)(generator);
        long double cumulative = 0;
        size_t lastPositive = 0;
        for (size_t i = 0; i < weightedItems.size(); ++i) {
            if (!weightedItems[i]) continue;
            lastPositive = i;
            cumulative += weightedItems[i];
            if (draw < cumulative) return i;
        }
        return lastPositive;
    }
    if (upper == 0) throw std::invalid_argument("Weighted sampling requires a positive weight");
    uint64_t sum = 0;
    uint64_t random = uint64Range(0, upper-1);
    for (size_t i = 0; i < weightedItems.size(); i++){
        sum += weightedItems[i];
        if (random < sum)
            return i;
    }
    return weightedItems.size()-1;
}

size_t RandomGenerator::takenIndex(const std::vector<uint64_t> &&weightedItems){
    return takenIndex(static_cast<const std::vector<uint64_t>&>(weightedItems));;
}

double RandomGenerator::doubleRange(double min, double max){
    return std::uniform_real_distribution<double>(min, max)(generator);
}

uint64_t RandomGenerator::uint64Range(uint64_t min, uint64_t max){
    return std::uniform_int_distribution<uint64_t>(min, max)(generator);
}

void RandomGenerator::setSeed(unsigned int seed){
    generator.seed(seed);
    this->seed = seed;
}

unsigned int RandomGenerator::getSeed() const{
    return seed;
}

void RandomGenerator::reset(){
    setSeed(getSeed());
}

unsigned int RandomGenerator::generateSeed(){
    std::random_device rd;
    unsigned int seed = rd();
    setSeed(seed);
    return seed;
}
