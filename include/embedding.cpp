#include "embedding.h"

//void EmbeddablePlane::embedGraph(const Graph &graph){
//}

void EmbeddablePlane::embedNode(Node node, double x, double y)
{
    addSpot(Spot(x, y, node.getID()));
}