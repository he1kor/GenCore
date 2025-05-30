#pragma once
#include <vector>

class Node{
    public:
        Node(int id);
        Node(int id, std::vector<int> neighbours);
        void addNeighbour(int id);
        void removeNeighbour(int id);
        const std::vector<int>& getNeighbours();
        int getID();
    private:
        int id;
        std::vector<int> neighbours;
};