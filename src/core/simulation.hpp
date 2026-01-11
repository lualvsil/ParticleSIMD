#pragma once

#include <span>
#include <vector>
#include <memory>

#define NUM_PARTICLES 3400

class Simulation {
private:
    int bodyCount;
    
    // Particles position
    std::vector<float> x;
    std::vector<float> y;
    
    std::vector<float> vx;
    std::vector<float> vy;
    
public:
    Simulation(int bodyCount, int width, int height);
    void update(float dt);
    std::span<const float> getX();
    std::span<const float> getY();
};
