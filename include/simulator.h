#pragma once

enum class SimulationStatus{
    INIT,
    RUNNING,
    FINISHED
};

class Simulator{
    public:
        Simulator(){};
        virtual ~Simulator() = default;

        Simulator(const Simulator&) = default;
        Simulator& operator=(const Simulator&) = default;

        Simulator(Simulator&&) = default;
        Simulator& operator=(Simulator&&) = default;

        virtual void start() final;
        virtual void step() final;
        virtual void finish() final;
        virtual void reset() final;

        bool isInitialized() const{return status == SimulationStatus::INIT;};
        bool isRunning() const{return status == SimulationStatus::RUNNING;};
        bool isFinished() const{return status == SimulationStatus::FINISHED;};

        long long getStep() const;
        SimulationStatus getStatus() const;

        static constexpr long long STARTING_STEP = 0;
        static constexpr long long NOT_STARTED_STEP = -1;

    protected:
        virtual void onStart(){};
        virtual void onStep() = 0;
        virtual void onFinish(){};
        virtual void onReset(){};
    private:
        long long steps = NOT_STARTED_STEP;
        SimulationStatus status = SimulationStatus::INIT;
};