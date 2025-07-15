#pragma once

enum class SimulationStatus{
    INIT,
    RUNNING,
    STOPPED
};

class Simulator{
    public:
        Simulator();
        virtual void start() final;
        virtual void step() final;
        virtual void finish() final;
        virtual void reset() final;
        virtual void onStart(){};
        virtual void onStep() = 0;
        virtual void onFinish(){};
        virtual void onReset(){};


        int getStep() const;
        SimulationStatus getStatus() const;
    private:
        int steps = -1;
        SimulationStatus status = SimulationStatus::INIT;
};