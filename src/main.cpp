#include <ogdf/energybased/FMMMLayout.h> // Force-directed layout
#include <ogdf/fileformats/GraphIO.h>
#include <ogdf/basic/Graph.h>
#include <ogdf/basic/graph_generators.h>
#include <iostream>

int main() {
    ogdf::Graph G;
    ogdf::randomSimpleGraph(G, 10, 20);

    ogdf::GraphAttributes GA(G, ogdf::GraphAttributes::nodeGraphics | ogdf::GraphAttributes::edgeGraphics);

    // Apply force-directed layout
    ogdf::FMMMLayout fmmm;
    fmmm.call(GA);

    // Export to SVG
    ogdf::GraphIO::write(GA, "graph_layout.svg", ogdf::GraphIO::drawSVG);

    return 0;
}