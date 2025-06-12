#include "edge_graph.h"

EdgeGraph::EdgeGraph(){}

void EdgeGraph::initializeEdgeIDs(){
    int max_edge_id = 0;
    for (Node node1 : nodes){
        for (Node node2 : node1.getNeighbours()){
            if (edgeIDs.count(std::make_pair(node2.getID(), node1.getID()))){
                edgeIDs[std::make_pair(node1.getID(), node2.getID())] = edgeIDs.at(std::make_pair(node2.getID(), node1.getID()));
                continue;
            }
            edgeIDs[std::make_pair(node1.getID(), node2.getID())] = max_edge_id;
            max_edge_id++;
        }
    }
}

EdgeGraph::EdgeGraph(std::vector<std::vector<int>> idGraph) : Graph(idGraph){
    initializeEdgeIDs();
}

EdgeGraph::EdgeGraph(std::vector<Node> nodes) : Graph(nodes){
    initializeEdgeIDs();
}

void EdgeGraph::removeEdge(int node1, int node2){
    Graph::removeEdge(node1, node2);
    edgeIDs.erase({node1, node2});
    edgeIDs.erase({node2, node1});
}


void EdgeGraph::separate(int id){
    for (int neighbourID : getNeighbours(id)){
        edgeIDs.erase({neighbourID, id});
        edgeIDs.erase({id, neighbourID});
    }
    Graph::separate(id);
}