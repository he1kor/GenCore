#pragma once

class Sizable{
    public:
        Sizable(){};
        Sizable(double radius) : radius{radius}{};
    private:
        double radius = 0;
};