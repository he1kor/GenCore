#pragma once

#include <random>

class RandomGenerator{
    public:
        RandomGenerator(const RandomGenerator&) = delete;
        RandomGenerator& operator=(const RandomGenerator&) = delete;
        static RandomGenerator& instance();
        bool chanceOccurred(double probability);
        void setSeed(unsigned int seed);
        unsigned int getSeed() const;
        unsigned int generateSeed();
    private:
        unsigned int seed;
        std::mt19937 generator;
        RandomGenerator();
};