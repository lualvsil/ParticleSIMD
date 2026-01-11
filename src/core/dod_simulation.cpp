#include "simulation.hpp"
#include "core/random.hpp"

#include <chrono>
#include <cmath>
#include <arm_neon.h>

Simulation::Simulation(int bodyCount, int width, int height)
    : x(bodyCount), y(bodyCount), bodyCount(bodyCount),
      vx(bodyCount), vy(bodyCount) {
    
    init_random();
    
    for (int i=0; i<bodyCount; i++) {
        float rx = genRandom01();
        float ry = genRandom01();
        
        x[i] = rx * width;
        y[i] = ry * height;
    }
}

void Simulation::update(float dt) {
    constexpr float G = 250.0f;
    constexpr float minNonZero = 0.0001f;

    float32x4_t n05V = vdupq_n_f32(0.5f);
    float32x4_t n15V = vdupq_n_f32(1.5f);
    
    for (int i = 0; i < bodyCount; i++) {
        float xi = x[i];
        float yi = y[i];
        float vxi = vx[i];
        float vyi = vy[i];
        
        float32x4_t forceAccumulatorX = vdupq_n_f32(0.0f);
        float32x4_t forceAccumulatorY = vdupq_n_f32(0.0f);
        
        float32x4_t xiV = vdupq_n_f32(xi);
        float32x4_t yiV = vdupq_n_f32(yi);
        
        for (int j = i+1; j < bodyCount; j+=4) {
            // Load the next 4 particlea into vector register
            float32x4_t xV = vld1q_f32(&x[j]);
            float32x4_t yV = vld1q_f32(&y[j]);
            
            // d = xj - xi
            float32x4_t dxV = vsubq_f32(xV, xiV);
            float32x4_t dyV = vsubq_f32(yV, yiV);
            
            // d2 = d*d
            float32x4_t dx2V = vmulq_f32(dxV, dxV);
            float32x4_t dy2V = vmulq_f32(dyV, dyV);
            
            // r2 -> total distance
            float32x4_t r2V0 = vaddq_f32(dx2V, dy2V);
            float32x4_t r2V = vaddq_f32(r2V0, vdupq_n_f32(minNonZero));
            
            // Approximate invR = 1 / sqrt(r2)
            float32x4_t invrV_ = vrsqrteq_f32(r2V);
            float32x4_t invrV;
            // Newthon-Rapshon algorithm
            {
                // y1 = y0 * (1.5 - 0.5 * r2 * y0 * y0)
                // y1 = y0 * (1.5 - 0.5 * r2 * [y0 * y0])
                float32x4_t y0y0 = vmulq_f32(invrV_, invrV_);
                // y1 = y0 * (1.5 - 0.5 * [r2 * ()])
                float32x4_t r2y0y0 = vmulq_f32(r2V, y0y0);
                // y1 = y0 * (1.5 - [0.5 * ()])
                
                float32x4_t n05r2y0y0 = vmulq_f32(r2y0y0, n05V);
                // y1 = y0 * [1.5 - ()])
                
                float32x4_t n15sub = vsubq_f32(n15V, n05r2y0y0);
                // y1 = y0 * ()
                invrV = vmulq_f32(n15sub, invrV_);
            }
            
            // F = [G * 20] / r2
            float32x4_t resultV = vdupq_n_f32(20.0f*G);
            // F = [() / r2]
            float32x4_t forceV = vdivq_f32(resultV, r2V);
            
            // finv = F * invR
            float32x4_t finvV_ = vmulq_f32(forceV, invrV);
            float32x4_t finvV = vmulq_n_f32(finvV_, dt); // multiply by delta time
            
            // F = F * d
            float32x4_t fxV = vmulq_f32(finvV, dxV);
            float32x4_t fyV = vmulq_f32(finvV, dyV);
            
            float32x4_t vxV = vld1q_f32(&vx[j]);
            float32x4_t vyV = vld1q_f32(&vy[j]);
            
            // v -= f
            float32x4_t vxResV = vsubq_f32(vxV, fxV);
            float32x4_t vyResV = vsubq_f32(vyV, fyV);
            vst1q_f32(&vx[j], vxResV);
            vst1q_f32(&vy[j], vyResV);
            
            forceAccumulatorX = vaddq_f32(fxV, forceAccumulatorX);
            forceAccumulatorY = vaddq_f32(fyV, forceAccumulatorY);
        }
        
        float scalarForceX = vaddvq_f32(forceAccumulatorX);
        float scalarForceY = vaddvq_f32(forceAccumulatorY);
        
        vx[i] += scalarForceX;
        vy[i] += scalarForceY;
    }
    // Escalar
    /*for (int i = 0; i < bodyCount; i+=1) {
        for (int j = i + 1; j < bodyCount; j++) {
            float dx = x[j] - x[i];
            float dy = y[j] - y[i];
            
            float r2 = dx*dx + dy*dy + minNonZero;
            float invR = 1.0f / std::sqrt(r2);
            
            float force = G * 20.0f / r2;
            
            
            float fx = force * dx * invR;
            float fy = force * dy * invR;
            
            vx[i] += fx * dt;
            vy[i] += fy * dt;
            
            vx[j] -= fx * dt;
            vy[j] -= fy * dt;
            
        }
    }*/

    for (int i = 0; i < bodyCount; i++) {
        x[i] += vx[i] * dt;
        y[i] += vy[i] * dt;
    }
}

std::span<const float> Simulation::getX() { return this->x; }
std::span<const float> Simulation::getY() { return this->y; }