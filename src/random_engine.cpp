#include "random_engine.h"

#include <stdexcept>
#include <random>

RandomEngine::RandomEngine(){
    generateSeed();
}

bool RandomEngine::chanceOccurred(double probability){
    if (probability < 0.0 || probability > 1.0) {
        throw std::invalid_argument("Probability must be between 0.0 and 1.0");
    }

    static std::mt19937 gen(seed);
    
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    
    return dist(gen) < probability;
}

void RandomEngine::setSeed(unsigned int seed){
    this->seed = seed;
}

unsigned int RandomEngine::getSeed() const{
    return seed;
}

unsigned int RandomEngine::generateSeed(){
    std::random_device rd;
    unsigned int seed = rd();
    setSeed(seed);
    return seed;
}
