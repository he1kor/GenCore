#include "multizone_resource_generator.h"
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

enum class Material { ROCK, SAND, SPICE, NONE };
using Mapping = ResourceMapping<Material>;
using Zones = std::unordered_map<Identifiable, ResourceGenerator<Material>, IDHash>;
using Masks = std::unordered_map<Identifiable, Matrix<double>, IDHash>;

static void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

static ResourceGenerator<Material> generator(const std::vector<Mapping>& resources) {
    Matrix<double> noise(16, 1);
    for (int x = 0; x < 16; ++x) noise.set(x, 0, x);
    ResourceGenerator<Material> result;
    result.setup({noise}, {1.0}, resources, {16, 1});
    return result;
}

static Matrix<Material> generate(const Zones& zones, const Masks& masks) {
    MultizoneResourceGenerator<Material> result;
    result.setup(zones, masks);
    return result.generateResourcesMap(Material::NONE);
}

static void same(const Matrix<Material>& a, const Matrix<Material>& b) {
    for (int x = 0; x < 16; ++x) require(a.get(x, 0) == b.get(x, 0), "Resource ordering changed output");
}

static void rejects(const std::vector<Mapping>& resources) {
    bool rejected = false;
    try { generator(resources); } catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "Invalid resource coverage was accepted");
}

int main() {
    try {
        std::vector<Mapping> a{{0.0, 0.4, Material::SAND}, {0.4, 1.0, Material::SPICE}};
        std::vector<Mapping> b{{0.0, 0.2, Material::ROCK}, {0.2, 0.7, Material::SAND}, {0.7, 1.0, Material::SPICE}};
        Masks masks{{1, Matrix<double>(16, 1, 0.35)}, {2, Matrix<double>(16, 1, 0.65)}};
        const auto baseline = generate({{1, generator(a)}, {2, generator(b)}}, masks);
        std::reverse(a.begin(), a.end());
        same(baseline, generate({{1, generator(a)}, {2, generator(b)}}, masks));
        std::reverse(b.begin(), b.end());
        same(baseline, generate({{2, generator(b)}, {1, generator(a)}}, masks));

        const auto rock = generator({{0.0, 1.0, Material::ROCK}});
        const auto sand = generator({{0.0, 1.0, Material::SAND}});
        const auto mixed = generate({{1, rock}, {2, sand}}, {{1, Matrix<double>(16, 1, 0.5)}, {2, Matrix<double>(16, 1, 0.5)}});
        int rocks = 0, sands = 0;
        for (int x = 0; x < 16; ++x) {
            rocks += mixed.get(x, 0) == Material::ROCK;
            sands += mixed.get(x, 0) == Material::SAND;
        }
        require(rocks == 8 && sands == 8, "Different material sets did not blend by identity");

        const auto unmasked = generate({{1, rock}}, {{1, Matrix<double>(16, 1, 0.0)}});
        for (int x = 0; x < 16; ++x) require(unmasked.get(x, 0) == Material::NONE, "Unmasked cells silently became rock");
        const auto explicitRock = generate({{1, rock}}, {{1, Matrix<double>(16, 1, 1.0)}});
        for (int x = 0; x < 16; ++x) require(explicitRock.get(x, 0) == Material::ROCK, "Explicit rock was lost");

        // With one active zone, preserve the actual percentile intervals, even if
        // their material order differs from the stable order used at blend cells.
        auto reversedIntervals = generator({{0.0, 0.5, Material::SPICE}, {0.5, 1.0, Material::ROCK}});
        same(reversedIntervals.generateResourcesMap(), generate({{1, reversedIntervals}}, {{1, Matrix<double>(16, 1, 1.0)}}));

        auto reused = rock;
        Matrix<double> noise(16, 1, 0.0);
        reused.setup({noise}, {1.0}, std::vector<Mapping>{{0.0, 1.0, Material::SAND}}, {16, 1});
        require(reused.getResourceMappings().size() == 1 && reused.getResourceThresholds().size() == 1, "Setup retained previous mappings");
        require(reused.generateResourcesMap().get(0, 0) == Material::SAND, "Reused generator selected an old resource");

        rejects({});
        rejects({{0.0, 0.0, Material::SAND}});
        rejects({{0.0, 0.3, Material::SAND}, {0.5, 1.0, Material::SPICE}});
        rejects({{0.0, 0.6, Material::SAND}, {0.5, 1.0, Material::SPICE}});
        rejects({{-0.1, 1.0, Material::SAND}});
        rejects({{0.0, std::numeric_limits<double>::quiet_NaN(), Material::SAND}});
        std::cout << "Resource identity, sparse lists, explicit rock, empty cells, reuse and validation checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
