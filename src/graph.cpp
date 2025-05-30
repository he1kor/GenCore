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

void Node::clearNeighbours(){
    neighbours.clear();
}

const std::set<int> &Node::getNeighbours(){
    return neighbours;
}

Graph::Graph(){}

Graph::Graph(std::vector<Node> nodes){
    this->nodes.reserve(nodes.size());
    for (auto node : nodes){
        checkID(node.getID(), OUT_OF_SIZE);
        nodes[node.getID()] = node;
    }
}

void Graph::separate(int separated_id){
    checkID(separated_id, NON_EXISTING);
    for (int node_id : nodes[separated_id].getNeighbours()){
        nodes[node_id].removeNeighbour(separated_id);
    }
    nodes[separated_id].clearNeighbours();
}

void Graph::insertNode(Node node){
    checkID(node.getID(), OUT_OF_SIZE);
    nodes[node.getID()] = node;
}

void Graph::checkID(int id, CheckIDVariant checkIDVariant){
    if (id > nodes.size() || id < 0){
        switch (checkIDVariant){
            case NON_EXISTING:
                throw std::runtime_error("The Node ID is not in the graph (given Node ID is out of bounds)");
                break;
            case OUT_OF_SIZE:
                throw std::runtime_error("The Node ID is not conconsecutive (given Node ID is out of bounds)");
                break;
        }
    }
}
