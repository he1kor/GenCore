#pragma once

enum class SimulationStatus{
    INIT,
    RUNNING,
    STOPPED
};

class Simulator{
    public:
        Simulator();
        virtual void start();
        virtual void step();
        virtual void finish();
        virtual void reset();
        int getStep();
        SimulationStatus getStatus();
    private:
        int steps = -1;
        SimulationStatus status = SimulationStatus::INIT;
};