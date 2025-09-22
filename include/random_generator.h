#pragma once

#include <random>

class RandomGenerator{
    public:
        RandomGenerator(const RandomGenerator&) = delete;
        RandomGenerator& operator=(const RandomGenerator&) = delete;
        static RandomGenerator& instance();
        bool chanceOccurred(double probability);
        size_t takenIndex(const std::vector<uint64_t>& weightedItems);
        size_t takenIndex(const std::vector<uint64_t>&& weightedItems);
        double doubleRange(double min, double max);
        uint64_t uint64Range(uint64_t min, uint64_t max);
        void setSeed(unsigned int seed);
        void reset();
        unsigned int getSeed() const;
        unsigned int generateSeed();
    private:
        unsigned int seed;
        std::mt19937 generator;
        RandomGenerator();
};


