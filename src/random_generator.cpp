#include "random_generator.h"

#include <stdexcept>
#include <numeric>

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
    uint64_t upper = std::accumulate(weightedItems.begin(), weightedItems.end(), 0);
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
