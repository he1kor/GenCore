#include "simulator.h"

#include <stdexcept>

Simulator::Simulator(){};

void Simulator::start(){
    if (status != SimulationStatus::INIT){
        throw std::logic_error("Can't start, the status is not INIT!");
    }
    status = SimulationStatus::RUNNING;
    steps = 0;
    onStart();
}

void Simulator::step(){
    if (status != SimulationStatus::RUNNING)
        throw std::logic_error("Can't step, the status is not RUNNING!");
    steps++;
    onStep();
}

void Simulator::finish(){
    if (status != SimulationStatus::RUNNING)
        throw std::logic_error("Can't finish, the status is not RUNNING!");
    status = SimulationStatus::STOPPED;
    onFinish();
}

void Simulator::reset(){
    if (status != SimulationStatus::STOPPED)
        throw std::logic_error("Can't reset, the status is not STOPPED!");
    status = SimulationStatus::INIT;
    steps = -1;
    onReset();
}

int Simulator::getStep() const{
    return steps;
}

SimulationStatus Simulator::getStatus() const{
    return status;
}
