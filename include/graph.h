#pragma once
#include <vector>
#include <set>

class Node{
    public:
        Node(int id);
        Node(int id, std::vector<int> neighbours);
        void addNeighbour(int id);
        void removeNeighbour(int id);
        void clearNeighbours();
        const std::set<int>& getNeighbours();
        int getID();
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
        Graph(std::vector<Node> nodes);
        void separate(int id);
        void insertNode(Node node);
    private:
        //indexed by ID
        std::vector<Node> nodes;
        void checkID(int id, CheckIDVariant checkIDVariant);
};