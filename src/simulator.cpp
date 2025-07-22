#include "simulator.h"

#include <stdexcept>

void Simulator::start(){
    if (status != SimulationStatus::INIT){
        throw std::logic_error("Can't start, the status is not INIT!");
    }
    status = SimulationStatus::RUNNING;
    steps = STARTING_STEP;
    onStart();
}

bool Simulator::step(){
    if (status != SimulationStatus::RUNNING)
        throw std::logic_error("Can't step, the status is not RUNNING!");
    steps++;
    onStep();
    return isRunning();
}

void Simulator::finish(){
    if (status != SimulationStatus::RUNNING)
        throw std::logic_error("Can't finish, the status is not RUNNING!");
    status = SimulationStatus::FINISHED;
    onFinish();
}

void Simulator::reset(){
    if (status != SimulationStatus::FINISHED)
        throw std::logic_error("Can't reset, the status is not FINISHED!");
    status = SimulationStatus::INIT;
    steps = NOT_STARTED_STEP;
    onReset();
}

long long Simulator::getStep() const{
    return steps;
}

SimulationStatus Simulator::getStatus() const{
    return status;
}
