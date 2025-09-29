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
 * 1. Embeds the template into a plane with force-directed layout.
 * 2. Rasterizes the plane into a grid.
 * 3. Expands zones via Voronoi bloating.
 */
std::shared_ptr<Grid<BasicNode>> generateMap() {
    // --- Phase 1: Graph Embedding ---
    EmbeddablePlane<BasicNode> embedding(128, 128);
    embedding.initEmbed(templates::grid3x3);  // Predefined 3x3 grid

    while (embedding.stepForceDirected()) {  // Runs until layout stabilizes
        #if 0  // Debug: Uncomment to log positions
        for (const auto& id : embedding.getIDs()) { ... }
        #endif
    }

    // --- Phase 2: Rasterization ---
    auto map = std::make_shared<Grid<BasicNode>>(safeRasterizePlane(embedding));

    // --- Phase 3: Zone Expansion ---
    ZoneBloater<BasicNode> zoneBloater;
    zoneBloater.initVoronoi(templates::grid3x3, map);
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
