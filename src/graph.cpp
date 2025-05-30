#include "graph.h"
#include <algorithm>

Node::Node(int id){
    this->id = id;
}

Node::Node(int id, std::vector<int> neighbours){
    this->id = id;
    this->neighbours = neighbours;
}

void Node::addNeighbour(int id){
    neighbours.push_back(id);
}

void Node::removeNeighbour(int id){
    neighbours.erase(std::remove(neighbours.begin(), neighbours.end(), id), neighbours.end());
}

const std::vector<int> &Node::getNeighbours(){
    return neighbours;
}
