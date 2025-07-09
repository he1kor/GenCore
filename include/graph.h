#pragma once

#include <vector>
#include <set>

#include "identifiable.h"

template<typename T>
class Node{
    static_assert(std::is_base_of_v<Identifiable, T>, "T must inherit from Identifiable");
    public:
        Node();
        Node(const T& id);
        Node(const T& t, std::vector<Identifiable> neighbours);
        void addNeighbour(const T& t);
        void removeNeighbour(const T& t);
        void clearNeighbours();
        const std::set<Identifiable>& getNeighbours() const;
        int getID() const;
    private:
        T t;
        std::set<Identifiable> neighbours;
};

enum CheckIDVariant {
    NON_EXISTING,
    OUT_OF_SIZE
};

class Graph{
    public:
        Graph();
        Graph(std::vector<std::vector<int>> idGraph);
        Graph(std::vector<Node> nodes);
        virtual void removeEdge(int node1, int node2);
        virtual void separate(int id);
        const Node& getNode(int id) const;
        const std::set<int>& getNeighbours(int id) const;
        int size() const;
    protected:
        //indexed by ID
        std::unordered_map<int, T> nodes;
    private:
        void checkID(int id, CheckIDVariant checkIDVariant);
};