#include "planar_symmetry.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <map>
#include <random>

namespace planar {

bool Layout::validRasterCandidate(const std::vector<DoubleVector2>& positions) const {
    if(!validRasterDrawing(positions,topology->edges,left,top,width,height))return false;
    if(!topology->nodeBoundary.empty()) {
        std::vector<DoubleVector2> raster;
        for(auto p:positions)raster.emplace_back(std::floor(p.x-left),std::floor(p.y-top));
        if(!validBoundaryDrawing(positions,*topology)||!validBoundaryDrawing(raster,*topology))return false;
    }
    if(!reflection||!reflection->gridAligned)return true;
    const int columns=static_cast<int>(width),rows=static_cast<int>(height);
    const auto axis=tileAxisForNormal(symmetryNormal,width,height);
    if (!axis) return false;
    std::map<std::pair<int,int>,size_t> seeds;
    for(size_t i=0;i<positions.size();++i) {
        int x=static_cast<int>(std::floor(positions[i].x-left)),y=static_cast<int>(std::floor(positions[i].y-top));
        size_t owner=i;
        const auto mirrored=reflectedTile({x,y},*axis,columns,rows);
        if (mirrored.x==x && mirrored.y==y && reflection->partner[i]!=i) return false;
        if(!canonicalTile({x,y},mirrored)) {
            x=mirrored.x;y=mirrored.y;
            owner=reflection->partner[i];
        }
        const auto [it,added]=seeds.emplace(std::make_pair(x,y),owner);
        if(!added&&it->second!=owner)return false;
    }
    return true;
}

DoubleVector2 Layout::reflectedVector(DoubleVector2 vector) const {
    const double dot = vector.x * symmetryNormal.x + vector.y * symmetryNormal.y;
    return {vector.x - 2 * dot * symmetryNormal.x, vector.y - 2 * dot * symmetryNormal.y};
}

void Layout::symmetricVectors(std::vector<DoubleVector2>& vectors) const {
    for (size_t i = 0; i < vectors.size(); ++i) {
        const size_t j = reflection->partner[i];
        if (i > j) continue;
        const auto other = reflectedVector(vectors[j]);
        vectors[i] = {(vectors[i].x + other.x) / 2, (vectors[i].y + other.y) / 2};
        vectors[j] = reflectedVector(vectors[i]);
    }
}

void Layout::coupleLimits(const std::vector<DoubleVector2>& proposed, std::vector<DoubleVector2>& limited) const {
    std::vector<double> fraction(proposed.size(), 1);
    for (size_t i = 0; i < proposed.size(); ++i) {
        const double squared = proposed[i].x * proposed[i].x + proposed[i].y * proposed[i].y;
        if (squared > 0) fraction[i] = std::clamp(
            (limited[i].x * proposed[i].x + limited[i].y * proposed[i].y) / squared, 0.0, 1.0);
    }
    for (size_t i = 0; i < proposed.size(); ++i) {
        const double value = std::min(fraction[i], fraction[reflection->partner[i]]);
        limited[i] = {proposed[i].x * value, proposed[i].y * value};
    }
}

void Layout::limitSymmetricBounds(std::vector<DoubleVector2>& movement) const {
    const auto proposed = movement;
    const double maxStep = std::min(width, height) * .02 * (1.0 - .8 * iteration / iterations);
    for (size_t i = 0; i < movement.size(); ++i) {
        const double length = std::hypot(movement[i].x, movement[i].y);
        if (!std::isfinite(length)) throw std::invalid_argument("Non-finite symmetric embedding force");
        double fraction = length > maxStep ? maxStep / length : 1;
        if (movement[i].x < 0) fraction = std::min(fraction, (points[i].x - left - .5) / -movement[i].x);
        if (movement[i].x > 0) fraction = std::min(fraction, (left + width - .5 - points[i].x) / movement[i].x);
        if (movement[i].y < 0) fraction = std::min(fraction, (points[i].y - top - .5) / -movement[i].y);
        if (movement[i].y > 0) fraction = std::min(fraction, (top + height - .5 - points[i].y) / movement[i].y);
        fraction = std::clamp(fraction,0.0,1.0);
        movement[i].x *= fraction; movement[i].y *= fraction;
    }
    coupleLimits(proposed,movement);
}

void Layout::initializeSymmetric(std::uint64_t seed) {
    const size_t n = topology->ids.size();
    if (reflection->partner.size() != n || reflection->drawing.size() != n
        || !validDrawing(reflection->drawing,topology->edges)
        || !validBoundaryDrawing(reflection->drawing,*topology)) throw std::invalid_argument("Invalid symmetric drawing or boundary placement supplied");
    for (size_t i = 0; i < n; ++i)
        if (reflection->partner[i] >= n || reflection->partner[reflection->partner[i]] != i)
            throw std::invalid_argument("Invalid reflection pairing supplied");
    for (size_t i = 0; i < n; ++i) {
        const auto p=reflection->drawing[i], q=reflection->drawing[reflection->partner[i]];
        if (std::hypot(p.x+q.x,p.y-q.y)>1e-9) throw std::invalid_argument("Supplied drawing does not realize its reflection pairing");
    }
    for (auto [a,b] : topology->edges) {
        const auto u = reflection->partner[a], v = reflection->partner[b];
        if (!std::binary_search(topology->edges.begin(),topology->edges.end(),Edge{std::min(u,v),std::max(u,v)}))
            throw std::invalid_argument("Reflection pairing does not preserve graph edges");
    }
    std::vector<size_t> degree(n);
    for (auto [a,b] : topology->edges) { ++degree[a]; ++degree[b]; }
    const DoubleVector2 center{left + width / 2, top + height / 2};
    // Face selection must not optimize away seeded orientation. Use a separate
    // stream so this never consumes growth/noise randomness or shape jitter.
    const bool freeOrientation=!hasPositionMagnets && !reflection->constrainedAxis;
    std::mt19937_64 orientationRandom(seed^0x6a09e667f3bcc909ULL);
    std::vector<double> angles;
    if (reflection->constrainedAxis) angles.push_back(std::atan2(reflection->normal.y,reflection->normal.x));
    else if (freeOrientation) {
        if (reflection->gridAligned) {
            const int count=width==height?4:2;
            const int first=int(orientationRandom()%count);
            for(int i=0;i<count;++i)angles.push_back(((first+i)%count)*std::numbers::pi/count);
        } else {
            const double first=(orientationRandom()>>11)*0x1.0p-53*2*std::numbers::pi;
            // Later angles are fallbacks only if tile rounding rejects the
            // requested one, not competitors scored for a preferred direction.
            for(int i=0;i<16;++i)angles.push_back(first+2*std::numbers::pi*i/16);
        }
    }
    else {
        if (reflection->gridAligned) {
            const int count=width==height?4:2;
            for(int i=0;i<count;++i)angles.push_back(i*std::numbers::pi/count);
        }
        else for (int i = 0; i < 16; ++i) angles.push_back(2 * std::numbers::pi * i / 16);
        // Include the continuous best rotation suggested by unpaired magnets.
        for (double flip : {-1.0,1.0}) {
            double cosine = 0, sine = 0;
            for (size_t i = 0; i < n; ++i) if (magnetForces[i] > 0) {
                const auto p = reflection->drawing[i];
                const DoubleVector2 target{magnetTargets[i].x-center.x,magnetTargets[i].y-center.y};
                cosine += magnetForces[i] * (flip*p.x*target.x+p.y*target.y);
                sine += magnetForces[i] * (flip*p.x*target.y-p.y*target.x);
            }
            if (!reflection->gridAligned && (cosine != 0 || sine != 0)) angles.push_back(std::atan2(sine,cosine));
        }
    }
    // The two ends of the axis and the drawing's handedness belong to seeded
    // orientation too. Do not let tiny score differences choose them again.
    const std::vector<double> flipsX=freeOrientation
        ?std::vector<double>{(orientationRandom()&1)?-1.0:1.0}:std::vector<double>{-1.0,1.0};
    const std::vector<double> flipsY=freeOrientation
        ?std::vector<double>{(orientationRandom()&1)?-1.0:1.0}:std::vector<double>{-1.0,1.0};
    std::vector<DoubleVector2> best, geometric;
    double bestValue = std::numeric_limits<double>::infinity(), geometricValue = bestValue;
    DoubleVector2 bestNormal{1,0}, geometricNormal{1,0};
    const auto consider = [&](const std::vector<DoubleVector2>& candidate) {
        if (!validDrawing(candidate,topology->edges) || !validBoundaryDrawing(candidate,*topology)) return;
        for (auto p : candidate) if (p.x < left+.5 || p.x > left+width-.5 || p.y < top+.5 || p.y > top+height-.5) return;
        const double value = score(candidate);
        if (value < geometricValue) { geometricValue=value; geometric=candidate; geometricNormal=symmetryNormal; }
        if (value < bestValue && validRasterCandidate(candidate)) {
            bestValue=value; best=candidate; bestNormal=symmetryNormal;
        }
    };
    for (double angle : angles) {
      for (double flipX : flipsX) for (double flipY : flipsY) {
        const double cosine=std::cos(angle), sine=std::sin(angle);
        symmetryNormal = {cosine,sine};
        auto offsets = reflection->drawing;
        double maxX=0, maxY=0, numerator=0, denominator=0;
        for (size_t i = 0; i < n; ++i) {
            const auto p=offsets[i];
            offsets[i] = {cosine*flipX*p.x-sine*flipY*p.y,sine*flipX*p.x+cosine*flipY*p.y};
            maxX=std::max(maxX,std::abs(offsets[i].x)); maxY=std::max(maxY,std::abs(offsets[i].y));
            if (magnetForces[i] > 0) {
                numerator += magnetForces[i]*(offsets[i].x*(magnetTargets[i].x-center.x)+offsets[i].y*(magnetTargets[i].y-center.y));
                denominator += magnetForces[i]*(offsets[i].x*offsets[i].x+offsets[i].y*offsets[i].y);
            }
        }
        const double fit=std::min((width/2-1)/std::max(maxX,1e-9),(height/2-1)/std::max(maxY,1e-9));
        for (double scale : {fit*.92, denominator > 0 ? std::clamp(numerator/denominator,fit*.2,fit) : fit*.75}) {
            auto candidate=offsets;
            for (auto& p : candidate) p={center.x+scale*p.x,center.y+scale*p.y};
            consider(candidate);
            auto all=candidate, leaves=candidate;
            for (size_t i=0; i<n; ++i) {
                const size_t j=reflection->partner[i];
                if (i>j || (magnetForces[i]==0 && magnetForces[j]==0)) continue;
                auto goal=magnetForces[i]>0 ? DoubleVector2{magnetTargets[i].x-center.x,magnetTargets[i].y-center.y}
                    : reflectedVector({magnetTargets[j].x-center.x,magnetTargets[j].y-center.y});
                if (i==j) {
                    const auto other=reflectedVector(goal); goal={(goal.x+other.x)/2,(goal.y+other.y)/2};
                }
                const auto other=reflectedVector(goal);
                all[i]={center.x+goal.x,center.y+goal.y}; all[j]={center.x+other.x,center.y+other.y};
                if (degree[i]<=1 && degree[j]<=1) { leaves[i]=all[i]; leaves[j]=all[j]; }
            }
            consider(all); consider(leaves);
            // Move whole pairs between faces before refinement starts; never
            // teleport one side alone or relax through a surrounding cycle.
            if (validDrawing(leaves,topology->edges)) candidate=leaves;
            for (size_t i=0; i<n; ++i) if (i<=reflection->partner[i] && (magnetForces[i]>0 || magnetForces[reflection->partner[i]]>0)) {
                auto next=candidate; const size_t j=reflection->partner[i];
                next[i]=all[i]; next[j]=all[j];
                if (validDrawing(next,topology->edges)) { consider(next); candidate=std::move(next); }
            }
        }
      }
      if (freeOrientation && !best.empty()) break;
    }
    if (geometric.empty()) throw std::runtime_error("Could not fit a symmetric planar drawing in the map bounds");
    if (!best.empty()) { points=best; bestRasterPoints=best; bestScore=bestValue; symmetryNormal=bestNormal; }
    else { points=std::move(geometric); symmetryNormal=geometricNormal; }
}
} // namespace planar
