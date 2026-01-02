#include "noise.h"
#include "random_generator.h"
#include <cmath>
#include <numbers>


Matrix<double> EllipticalBlobNoise::generate() {
    Matrix<double> result(getWidth(), getHeight());
    
    for (double& pixel : result) {
        pixel = 0.5;
    }
    
    RandomGenerator& rng = RandomGenerator::instance();
    
    double avgRadius = (minRadius + maxRadius) / 2.0;
    int numSpots = static_cast<int>(
        (getWidth() * getHeight()) / (avgRadius * avgRadius)
    ) * 4;
    
    for (int n = 0; n < numSpots; ++n) {
        double cx = rng.doubleRange(0.0, getWidth());
        double cy = rng.doubleRange(0.0, getHeight());
        
        double intensity = rng.doubleRange(-0.3, 0.3);
        
        double rx = rng.doubleRange(minRadius, maxRadius);
        double ry = rng.doubleRange(minRadius, maxRadius);
        
        double angle = rng.doubleRange(0.0, std::numbers::pi);
        double cos_a = std::cos(angle);
        double sin_a = std::sin(angle);
        
        double max_radius = std::max(rx, ry) * 1.5;
        int min_i = std::max(0, static_cast<int>(cy - max_radius));
        int max_i = std::min(getHeight(), static_cast<int>(cy + max_radius) + 1);
        int min_j = std::max(0, static_cast<int>(cx - max_radius));
        int max_j = std::min(getWidth(), static_cast<int>(cx + max_radius) + 1);
        
        for (int i = min_i; i < max_i; ++i) {
            for (int j = min_j; j < max_j; ++j) {
                double px = j + 0.5;
                double py = i + 0.5;
                
                double dx = px - cx;
                double dy = py - cy;
                
                double dx_rot = dx * cos_a + dy * sin_a;
                double dy_rot = -dx * sin_a + dy * cos_a;
                
                double norm_dist_sq = (dx_rot / rx) * (dx_rot / rx) + 
                                     (dy_rot / ry) * (dy_rot / ry);
                
                if (norm_dist_sq <= 1.0) {
                    double falloff = 1.0 - norm_dist_sq;
                    double current = result.get(j, i);
                    result.set(j, i, current + intensity * falloff);
                }
            }
        }
    }
    
    return result;
}