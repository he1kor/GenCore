#include "simulator.h"

#include <stdexcept>

Simulator::Simulator(){};

void Simulator::start(){
    if (status != SimulationStatus::INIT){
        throw std::logic_error("Can't start, the simulation is not initialized!");
    }
    steps = 0;
    status = SimulationStatus::RUNNING;
}

void Simulator::step(){
    if (status != SimulationStatus::RUNNING)
        throw std::logic_error("Can't step, the simulation is not running!");
    steps++;
}

void Simulator::finish(){
    if (status != SimulationStatus::RUNNING)
        throw std::logic_error("Can't finish, the simulation is not running!");
    status = SimulationStatus::STOPPED;
}

void Simulator::reset(){
    if (status != SimulationStatus::STOPPED)
        throw std::logic_error("Can't reset, the simulation is not finished!");
    status = SimulationStatus::INIT;
}

int Simulator::getStep(){
    return steps;
}

SimulationStatus Simulator::getStatus(){
    return status;
}
