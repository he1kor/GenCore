#include "graph.h"
#include <algorithm>
#include <stdexcept>

Node::Node(int id){
    this->id = id;
}

Node::Node(int id, std::vector<int> neighbours){
    this->id = id;
    this->neighbours = std::set(neighbours.begin(), neighbours.end());
}

void Node::addNeighbour(int id){
    neighbours.insert(id);
}

void Node::removeNeighbour(int id){
    neighbours.erase(std::remove(neighbours.begin(), neighbours.end(), id), neighbours.end());
}

const std::set<int> &Node::getNeighbours(){
    return neighbours;
}

Graph::Graph(){}

Graph::Graph(std::vector<Node> nodes){
    this->nodes.reserve(nodes.size());
    for (auto node : nodes){
        if (node.getID() > nodes.size() || node.getID() < 0)
            throw std::runtime_error("The Node IDs are not conconsecutive (given Node ID is out of bounds)");
        nodes[node.getID()] = node;
    }
}