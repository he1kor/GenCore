#pragma once

#include <vector>
#include <set>

#include "identifiable.h"

template<typename T>
class Node{
    static_assert(std::is_base_of_v<Identifiable, T>, "T must inherit from Identifiable");
    public:
        Node(const T& value);
        Node(const T& value, std::vector<Identifiable> neighbours);
        void addNeighbour(const Identifiable& id);
        void removeNeighbour(const Identifiable& id);
        void clearNeighbours();
        const std::set<Identifiable>& getNeighbours() const;
        Identifiable getID() const;
    private:
        T value;
        std::set<Identifiable> neighbours;
};

enum CheckIDVariant {
    NON_EXISTING,
    OUT_OF_SIZE
};

template <typename T>
class Graph{
    public:
        static_assert(std::is_base_of_v<Identifiable, T>, "T must inherit from Identifiable");
        Graph();
        Graph(const std::vector<std::pair<T, std::vector<Identifiable>>>& rawGraph);
        Graph(std::vector<Node<T>> nodes);
        virtual void removeEdge(Identifiable node1, Identifiable node2);
        virtual void separate(Identifiable id);
        const Node<T>& getNode(Identifiable id) const;
        const std::set<Identifiable>& getNeighbours(Identifiable id) const;
        int size() const;
    protected:
        //indexed by ID
        std::unordered_map<Identifiable, Node<T>> nodes;
    private:
};

template<typename T>
Node<T>::Node(const T& value){
    this->value = value;
}

template<typename T>
Node<T>::Node(const T& value, std::vector<Identifiable> neighbours){
    this->value = value;
    this->neighbours = std::set(neighbours.begin(), neighbours.end());
}

template<typename T>
void Node<T>::addNeighbour(const Identifiable& id){
    neighbours.insert(id);
}

template<typename T>
void Node<T>::removeNeighbour(const Identifiable& id){
    neighbours.erase(id);
}

template<typename T>
void Node<T>::clearNeighbours(){
    neighbours.clear();
}

template<typename T>
const std::set<Identifiable> &Node<T>::getNeighbours() const{
    return neighbours;
}

template<typename T>
Identifiable Node<T>::getID() const{
    return id;
}




template<typename T>
Graph<T>::Graph(){}

template<typename T>
Graph<T>::Graph(const std::vector<std::pair<T, std::vector<Identifiable>>>& rawGraph){
    nodes.resize(idGraph.size());
    for (int i = 0; i < idGraph.size(); i++){
        nodes[rawGraph[i].first] = Node(rawGraph[i].first, rawGraph[i].second);
    }
}

template<typename T>
Graph<T>::Graph(std::vector<Node<T>> nodes){
    this->nodes.resize(nodes.size());
    for (const auto& node : nodes){
        this->nodes[node.getID()] = node;
    }
}

template<typename T>
void Graph<T>::removeEdge(Identifiable node1, Identifiable node2){
    nodes.at(node1).removeNeighbour(node2);
    nodes.at(node2).removeNeighbour(node1);
}

template<typename T>
void Graph<T>::separate(Identifiable separated_id){
    for (int node_id : nodes.at(separated_id).getNeighbours()){ 
        nodes[node_id].removeNeighbour(separated_id);
    }
    nodes[separated_id].clearNeighbours();
}

template<typename T>
const Node<T> &Graph<T>::getNode(Identifiable id) const{
    return nodes.at(id);
}

template<typename T>
const std::set<Identifiable> &Graph<T>::getNeighbours(Identifiable id) const{
    return getNode(id).getNeighbours();
}

template<typename T>
int Graph<T>::size() const{
    return nodes.size();
}