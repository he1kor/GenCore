# GenCore - Map Generation Library

Work-in-progress C++23 library for procedural finite map generation.

## Current features:
- **seed-defined RNG**: Regenerate identical maps with the same seed.
- **random graph embedding**: Used for placement of key map objects as well as zone system.
- **zone system**: Prepare a logistic graph of zones to bloat a map with specified regions and their connections.
- **step-by-step simulation**: Inspect each generation step for debugging/visualization.
- **zone and connection templates**: Extend the features to fit your requirements (e.g. biomes, roads)

## From Graph to Map
Graph embedding:

![demo](https://github.com/user-attachments/assets/017bf8c5-7ca0-4f19-917d-68f625d18965)

Zone bloat:

![demo 2](https://github.com/user-attachments/assets/09b12063-5f3b-4624-9d1b-5494754b1af5)

## Borders and passages generation
![demo 3](https://github.com/user-attachments/assets/502451ad-8c0d-492b-a178-14d4f4656873)


## Quick Example:

```cpp
#include <rasterization.h>
#include <templates.h>
#include <embedding.h>
#include <grid.h>
#include <zone_bloater.h>
/**
 * Generates a 128x128 map using a 3x3 grid template.
 * Steps:
 * 1. Constructs a planar drawing and refines its geometry safely.
 * 2. Rasterizes the plane into a grid.
 * 3. Expands zones via Voronoi bloating.
 */
std::shared_ptr<Grid<BasicNode>> generateMap() {
    // --- Phase 1: Graph Embedding ---
    EmbeddablePlane<BasicNode> embedding(128, 128);
    embedding.initEmbed(templates::grid3x3);  // Predefined 3x3 grid

    while (embedding.stepPlanar()) {  // Runs a bounded geometry refinement
        #if 0  // Debug: Uncomment to log positions
        for (const auto& id : embedding.getIDs()) { ... }
        #endif
    }

    // --- Phase 2: Rasterization ---
    auto map = std::make_shared<Grid<BasicNode>>(safeRasterizePlane(embedding));

    // --- Phase 3: Zone Expansion ---
    ZoneBloater<BasicNode, BasicSymConnection, BasicAsymConnection> zoneBloater;
    zoneBloater.initEdgeVoronoi(*templates::grid3x3, map);
    zoneBloater.setBloatMode(bloatStrategies::DiagonalRandomBloat());
    zoneBloater.start();
    while (zoneBloater.step()) {  // Progressively expands zones
        #if 0  // Debug: Uncomment to inspect tiles
        std::cout << "Progress: " << zoneBloater.getProgress() << "%\n";
        #endif
    }

    return map;  // Final generated map
}
```
Generation result:

<img width="256" height="256" alt="demo 3" src="https://github.com/user-attachments/assets/7a821efe-d1f4-4566-b7c6-094732f336ca" />

*Example output using `grid3x3` template* 

`templates::grid3x3` is a neutral nine-node grid with twelve undirected connections.
`templates::makeGrid3x3(susceptibility)` creates the same topology with a chosen
susceptibility for every node. Apply a magnet grid with `applyMagnetGrid` before
stepping the embedding to enable magnetic placement.

## Planar layout

`initEmbed(graph, preferences, seed, topology)` now constructs a crossing-free
drawing by default. Boyer-Myrvold checks planarity. A private augmented graph,
temporary face centers and a virtual outer frame support a weighted harmonic
initialization; Chrobak-Payne supplies an integer-drawing fallback. Added edges
and vertices never become gameplay connections or growth seeds.

Refinement uses PrEd-style separating half-planes for simultaneous node movement.
The drawing is independently checked with filtered exact orientation predicates
after each step. Completed results must also remain planar with distinct centers
after flooring to tile coordinates. Dense layouts that cannot be separated at
the requested tile resolution report a generation error.

`EdgeGeometryMap` maps an undirected endpoint-ID pair to
`EdgeGeometry{preferredCenterDistance, distanceWeight}`. Distances are soft
preferences in plane/tile units, and the default zero distance selects spacing
based on normalized node radii. Conflicting preferences are accepted. Magnets
also act through constrained movements. Radii, lengths and forces do not
guarantee final tile areas or passage widths.

The bounding box is centered without stretching requested distances. An explicit
local seed controls variation without consuming `RandomGenerator`'s stream.
`planar::makeTopology(graph)` can cache the immutable topology for repeated runs.
`embed(graph)` is the synchronous convenience entry point.

`initForceDirected(graph)` retains the old unconstrained initializer for research
and its regression tests. `stepForceDirected()` dispatches to the planar method
when initialized with `initEmbed`; new code should call `stepPlanar()` explicitly.

The private implementation uses Boost Graph and Multiprecision headers. CMake
finds installed headers via `GENCORE_BOOST_INCLUDE_DIR`, or downloads the pinned,
SHA-256-verified Boost 1.87 archive. No compiled Boost libraries are required.
The Boost headers retain their Boost Software License.

## Embedding diagnostics

`EmbeddablePlane<T>::findEdgeIntersections() const` checks all unique undirected
edge pairs in the current straight-line drawing in O(E^2) time. It returns a
vector of `EdgeIntersection` records, each containing `firstEdge` and
`secondEdge` endpoint ID pairs. Endpoints and results are sorted by ID for
repeatable tests. It can be called after initialization, during embedding, or
after completion, without changing the drawing. Before initialization it returns
an empty vector.

The checker reports proper crossings, collinear overlaps (including incident
edges), and contacts between edges without a shared graph vertex. Edges meeting
only at their shared graph vertex are allowed. Checks use floating-point plane
positions before rasterization; they do not validate connectivity, final zone
areas, passages, isolated vertices, or collisions introduced by rounding to tiles.

```cpp
const auto intersections = embedding.findEdgeIntersections();
for (const auto& hit : intersections) {
    std::cout << hit.firstEdge.first << " - " << hit.firstEdge.second
              << " intersects " << hit.secondEdge.first << " - "
              << hit.secondEdge.second << '\n';
}
```

`warnEdgeIntersections(std::ostream& output = std::cerr) const` prints a summary
and all offending pairs, returning the pair count. The legacy force-directed
step calls it automatically at completion. The default planar path enforces
validity throughout construction/refinement instead of warning and continuing.

Enable `GENCORE_BUILD_TESTS` to build `embedding_intersection_regression`, which
covers crossings, endpoint contacts, overlaps, shared vertices, close parallel
edges, collapsed segments, deterministic results, and completion warnings.
`planar_embedding_regression` additionally covers nonplanarity rejection,
trees, articulation points, disconnected and nested graphs, small grids,
preferences, centering, seed variation and independence from growth randomness.

## Resource mappings

`ResourceGenerator<T>` maps normalized noise percentiles to resource values supplied
by the caller. A mapping uses a half-open interval `[minimalThreshold, maximalThreshold)`:

```cpp
enum class ResourceType { TypeA, TypeB, Empty };

std::vector<ResourceMapping<ResourceType>> mappings = {
    {0.0, 0.4, ResourceType::TypeA},
    {0.4, 1.0, ResourceType::TypeB}
};
```

Positive intervals must cover `[0, 1]` without gaps or overlaps. Invalid coverage
raises `std::invalid_argument`. A uniform zone uses one resource mapping covering
the full range.

`MultizoneResourceGenerator<T>` matches resources by their values rather than their
positions in a list. Zones may list resources in different orders and omit unused
resource values. At cells influenced by one zone, its percentile intervals are used
directly. At blended cells, interval widths become resource shares, which are combined
by identity and sampled in stable resource order. The resource type `T` must support
ordering.

`MultizoneResourceGenerator<T>::generateResourcesMap(emptyResource)` requires an
explicit value for cells outside all zone masks. Resource identities, serialization,
and presentation are defined by the application using the library.

## Installation

### Using CMake

GenCore is built with CMake. To use it in your project:

1. **Clone the repository** or add it as a submodule to your project:
  ```bash
  git clone https://github.com/he1kor/GenCore.git
  ```

2. **Add to your CMake project:**
  ```CMake
  add_subdirectory(path/to/GenCore)
  target_link_libraries(your_target_name PUBLIC GenCore)
  ```
