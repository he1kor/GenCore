#include "graph.h"
#include <algorithm>
#include <stdexcept>

Node::Node(){
    id = -1;
}

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
    neighbours.erase(id);
}

void Node::clearNeighbours(){
    neighbours.clear();
}

const std::set<int> &Node::getNeighbours() const{
    return neighbours;
}

int Node::getID() const{
    return id;
}

Graph::Graph(){}

Graph::Graph(std::vector<std::vector<int>> idGraph){
    nodes.resize(idGraph.size());
    for (int i = 0; i < idGraph.size(); i++){
        nodes[i] = Node(i, idGraph[i]);
    }
}

Graph::Graph(std::vector<Node> nodes){
    this->nodes.resize(nodes.size());
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

const Node &Graph::getNode(int id) const{
    return nodes[id];
}

void Graph::insertNode(Node node){
    checkID(node.getID(), OUT_OF_SIZE);
    separate(node.getID());
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

int Graph::size() const{
    return nodes.size();
}