#include "simulation.hpp"
#include "core/random.hpp"

#include <chrono>
#include <cmath>
#include <arm_neon.h>

const float G = 10.0f;
const float particleMass = 50.0f;

static void _update_instrinsics(void* data, int begin, int end) {
    auto sim = static_cast<SimulationJobData*>(data);
    auto dt = sim->dt;
    
    // F = [G * 20] / r2
    static float32x4_t massTimesGV = vdupq_n_f32(particleMass*G);
    static float32x4_t zeroVector = vdupq_n_f32(0.0f);
    
    for (int i = begin; i < end; i++) {
        float xi = sim->x[i];
        float yi = sim->y[i];
        
        float32x4_t accX0 = vdupq_n_f32(0.0f);
        float32x4_t accX1 = vdupq_n_f32(0.0f);
        float32x4_t accY0 = vdupq_n_f32(0.0f);
        float32x4_t accY1 = vdupq_n_f32(0.0f);
        
        float32x4_t xiV = vdupq_n_f32(xi);
        float32x4_t yiV = vdupq_n_f32(yi);
        
        for (int j = 0; j < sim->length; j+=8) {
            // Load the next 4 particles into vector register
            float32x4_t xV0 = vld1q_f32(&sim->x[j]);
            float32x4_t xV1 = vld1q_f32(&sim->x[j+4]);
            float32x4_t yV0 = vld1q_f32(&sim->y[j]);
            float32x4_t yV1 = vld1q_f32(&sim->y[j+4]);
            
            // d = xj - xi
            float32x4_t dxV0 = vsubq_f32(xV0, xiV);
            float32x4_t dxV1 = vsubq_f32(xV1, xiV);
            float32x4_t dyV0 = vsubq_f32(yV0, yiV);
            float32x4_t dyV1 = vsubq_f32(yV1, yiV);
            
            // d2 = dx*dx + dy*dy
            float32x4_t r2V0 = vaddq_f32(vmulq_f32(dxV0, dxV0), vmulq_f32(dyV0, dyV0));
            float32x4_t r2V1 = vaddq_f32(vmulq_f32(dxV1, dxV1), vmulq_f32(dyV1, dyV1));
            
            // Mask
            // If i and j are the same particles, set the mask bits to 0, otherwise sets to 1
            uint32x4_t mask0 = vcgtq_f32(r2V0, zeroVector);
            uint32x4_t mask1 = vcgtq_f32(r2V1, zeroVector);
            
            // Approximate invR = 1 / sqrt(r2)
            float32x4_t invrV0_ = vrsqrteq_f32(r2V0);
            float32x4_t invrV1_ = vrsqrteq_f32(r2V1);
            // Newthon-Rapshon algorithm
            float32x4_t step0 = vrsqrtsq_f32(r2V0, vmulq_f32(invrV0_, invrV0_));
            float32x4_t step1 = vrsqrtsq_f32(r2V1, vmulq_f32(invrV1_, invrV1_));
            
            float32x4_t invrV0 = vmulq_f32(invrV0_, step0);
            float32x4_t invrV1 = vmulq_f32(invrV1_, step1);
            
            // F = [(G*mass) / r2]
            float32x4_t forceV0 = vdivq_f32(massTimesGV, r2V0);
            float32x4_t forceV1 = vdivq_f32(massTimesGV, r2V1);
            
            // finv = F * invR
            float32x4_t finvV0 = vmulq_f32(forceV0, invrV0);
            float32x4_t finvV1 = vmulq_f32(forceV1, invrV1);
            
            // Apply the mask, so if i and j are the same particles,
            // this will cancel the force.
            finvV0 = vreinterpretq_f32_u32(
                vandq_u32(vreinterpretq_u32_f32(finvV0),
                mask0)
            );
            finvV1 = vreinterpretq_f32_u32(
                vandq_u32(vreinterpretq_u32_f32(finvV1),
                mask1)
            );
            
            // F = F * d
            float32x4_t fxV0 = vmulq_f32(finvV0, dxV0);
            float32x4_t fxV1 = vmulq_f32(finvV1, dxV1);
            float32x4_t fyV0 = vmulq_f32(finvV0, dyV0);
            float32x4_t fyV1 = vmulq_f32(finvV1, dyV1);
            
            // Accumulate force in i for each interaction
            accX0 = vaddq_f32(fxV0, accX0);
            accX1 = vaddq_f32(fxV1, accX1);
            accY0 = vaddq_f32(fyV0, accY0);
            accY1 = vaddq_f32(fyV1, accY1);
        }
        
        float scalarForceX = vaddvq_f32(vaddq_f32(accX0, accX1)) * dt;
        float scalarForceY = vaddvq_f32(vaddq_f32(accY0, accY1)) * dt;
        
        sim->vx[i] += scalarForceX;
        sim->vy[i] += scalarForceY;
    }
}

static void _update_no_instrinsics(void* data, int begin, int end) {
    auto sim = static_cast<SimulationJobData*>(data);
    float dt = sim->dt;

    for (int i = begin; i < end; i++) {
        float xi = sim->x[i];
        float yi = sim->y[i];

        float accX = 0.0f;
        float accY = 0.0f;

        for (int j = 0; j < sim->length; j++) {
            float dx = sim->x[j] - xi;
            float dy = sim->y[j] - yi;

            float r2 = dx*dx + dy*dy + 1e-9f;

            float invr = 1.0f / sqrtf(r2);
            float force = (particleMass * G) / r2;
            float finv = force * invr;

            accX += finv * dx;
            accY += finv * dy;
        }

        sim->vx[i] += accX * dt;
        sim->vy[i] += accY * dt;
    }
}


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
    
    data = new SimulationJobData {
        .x=&x[0],
        .y=&y[0],
        .length=bodyCount,
        .vx=&vx[0],
        .vy=&vy[0],
    };
    
    jobSystem = new JobSystem(8);
    jobSystem->set_executor(_update_instrinsics);
    // jobSystem->set_executor(_update_no_instrinsics);
    
    int particlesPerJob = bodyCount / 128;
    jobSystem->set_data(data, bodyCount, particlesPerJob);
}


Simulation::~Simulation() {
    delete jobSystem;
    delete data;
}

void Simulation::update(float dt) {
    data->dt = dt;
    
    jobSystem->dispatch();
    jobSystem->wait();
    
    for (int i = 0; i < bodyCount; i++) {
        x[i] += vx[i] * dt;
        y[i] += vy[i] * dt;
    }
}

std::span<const float> Simulation::getX() { return this->x; }
std::span<const float> Simulation::getY() { return this->y; }