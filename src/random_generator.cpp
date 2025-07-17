#include "random_generator.h"

#include <stdexcept>

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

void RandomGenerator::setSeed(unsigned int seed){
    generator.seed(seed);
    this->seed = seed;
}

unsigned int RandomGenerator::getSeed() const{
    return seed;
}

unsigned int RandomGenerator::generateSeed(){
    std::random_device rd;
    unsigned int seed = rd();
    setSeed(seed);
    return seed;
}
