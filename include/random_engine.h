#pragma once

class RandomEngine{
    public:
        RandomEngine();
        bool chanceOccurred(double probability);
        void setSeed(unsigned int seed);
        unsigned int getSeed() const;
        unsigned int generateSeed();
    private:
        unsigned int seed;
};