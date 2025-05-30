#pragma once

#include "graph.h" 
#include "plane.h" 

class EmbeddablePlane : public Plane{
    public:
        //void embedGraph(const Graph& graph);
        void embedNode(Node node, double x, double y);
};