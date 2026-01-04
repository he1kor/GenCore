#pragma once

#include <vector>
#include <set>

#include "identifiable.h"

template<hasID T>
class Node{
    public:
        Node();
        Node(const T& value);
        Node(const T& value, std::vector<Identifiable> neighbours);
        void addNeighbour(const Identifiable& id);
        void removeNeighbour(const Identifiable& id);
        void clearNeighbours();
        const std::set<Identifiable>& getNeighbours() const;
        Identifiable getID() const;
        const T& getValue() const;
    private:
        T value;
        std::set<Identifiable> neighbours;
};

enum CheckIDVariant {
    NON_EXISTING,
    OUT_OF_SIZE
};

template <hasID T>
class Graph{
    public:
        Graph();
        Graph(const std::vector<std::pair<T, std::vector<Identifiable>>>& rawGraph);
        Graph(std::vector<Node<T>> nodes);
        virtual void removeEdge(Identifiable node1, Identifiable node2);
        virtual void separate(Identifiable id);
        int getSize() const;
        const Node<T>& getNode(Identifiable id) const;
        const T& getValue(Identifiable id) const;
        const std::set<Identifiable>& getNeighbours(Identifiable id) const;
        const std::vector<Identifiable>& getIDs() const;
        int size() const;
    protected:
        std::vector<Identifiable> ids;
        std::unordered_map<Identifiable, Node<T>, IDHash> nodes;
    private:
};

template<hasID T>
Node<T>::Node(){}

template<hasID T>
Node<T>::Node(const T &value){
    this->value = value;
}

template<hasID T>
Node<T>::Node(const T& value, std::vector<Identifiable> neighbours){
    this->value = value;
    this->neighbours = std::set(neighbours.begin(), neighbours.end());
}

template<hasID T>
void Node<T>::addNeighbour(const Identifiable& id){
    neighbours.insert(id);
}

template<hasID T>
void Node<T>::removeNeighbour(const Identifiable& id){
    neighbours.erase(id);
}

template<hasID T>
void Node<T>::clearNeighbours(){
    neighbours.clear();
}

template<hasID T>
const std::set<Identifiable> &Node<T>::getNeighbours() const{
    return neighbours;
}

template<hasID T>
Identifiable Node<T>::getID() const{
    return static_cast<Identifiable>(value);
}

template<hasID T>
const T& Node<T>::getValue() const{
    return value;
}




template<hasID T>
Graph<T>::Graph(){}

template<hasID T>
Graph<T>::Graph(const std::vector<std::pair<T, std::vector<Identifiable>>>& rawGraph){
    nodes.reserve(rawGraph.size());
    ids.reserve(rawGraph.size());
    for (int i = 0; i < rawGraph.size(); i++){
        nodes[rawGraph[i].first] = Node(rawGraph[i].first, rawGraph[i].second);
        ids.push_back(static_cast<Identifiable>(rawGraph[i].first));
    }
}

template<hasID T>
Graph<T>::Graph(std::vector<Node<T>> nodes_vec){
    nodes.reserve(nodes_vec.size());
    ids.reserve(nodes.size());
    for (const auto& node : nodes_vec){
        nodes.push_back(node);
        ids.push_back(static_cast<Identifiable>(node));
    }
}

template<hasID T>
void Graph<T>::removeEdge(Identifiable node1, Identifiable node2){
    nodes.at(node1).removeNeighbour(node2);
    nodes.at(node2).removeNeighbour(node1);
}

template<hasID T>
int Graph<T>::getSize() const{
    return nodes.size();
}

template<hasID T>
const std::vector<Identifiable>& Graph<T>::getIDs() const{
    return ids;
}

template<hasID T>
void Graph<T>::separate(Identifiable separated_id){
    for (Identifiable node_id : nodes.at(separated_id).getNeighbours()){ 
        nodes[node_id].removeNeighbour(separated_id);
    }
    nodes[separated_id].clearNeighbours();
}

template<hasID T>
const Node<T> &Graph<T>::getNode(Identifiable id) const{
    return nodes.at(id);
}

template<hasID T>
const std::set<Identifiable> &Graph<T>::getNeighbours(Identifiable id) const{
    return getNode(id).getNeighbours();
}


template<hasID T>
const T& Graph<T>::getValue(Identifiable ID) const{
    return getNode(ID).getValue();
}

template<hasID T>
int Graph<T>::size() const{
    return nodes.size();
}