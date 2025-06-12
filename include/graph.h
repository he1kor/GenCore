#pragma once
#include <vector>
#include <set>

class Node{
    public:
        Node();
        Node(int id);
        Node(int id, std::vector<int> neighbours);
        void addNeighbour(int id);
        void removeNeighbour(int id);
        void clearNeighbours();
        const std::set<int>& getNeighbours() const;
        int getID() const;
    private:
        int id;
        std::set<int> neighbours;
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
        std::vector<Node> nodes;
    private:
        void checkID(int id, CheckIDVariant checkIDVariant);
};