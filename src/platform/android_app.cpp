#include <android_native_app_glue.h>
#include <android/native_window.h>
#include <android/log.h>

#include "renderer/renderer.hpp"
#include "core/simulation.hpp"

#include <print>
#include <chrono>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "NativeEGL", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "NativeEGL", __VA_ARGS__)

typedef struct {
    int width;
    int height;
    
    int fpsPosX;
    int fpsPosY;
    
    android_app* app;
    Renderer* renderer;
    Simulation* simulation;
} AppState;

static void handle_cmd(struct android_app* app, int32_t cmd) {
    AppState* state = (AppState*)app->userData;

    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            state->renderer = new Renderer();
            state->width = ANativeWindow_getWidth(app->window);
            state->height = ANativeWindow_getHeight(app->window);
            state->width /= 2;
            state->height /= 2;
            state->fpsPosX = state->width / 6;
            state->fpsPosY = 0;
            ANativeWindow_setBuffersGeometry(app->window, state->width, state->height, AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM);
            state->simulation = new Simulation(NUM_PARTICLES, state->width, state->height);
            break;
        
        case APP_CMD_TERM_WINDOW:
            break;
    }
}

void on_frame(AppState* state, float dt, float fps) {
    if (state->renderer == nullptr || state->simulation == nullptr)
        return;
    if (state->app->window == nullptr)
        return;
    
    state->simulation->update(dt);
    ANativeWindow_Buffer buffer;
    ANativeWindow_lock(state->app->window, &buffer, nullptr);
    
    state->renderer->setBuffer(buffer.bits, buffer.stride, state->height);
    std::memset(buffer.bits, 0, buffer.stride*state->height*4);
    
    auto x=state->simulation->getX();
    auto y=state->simulation->getY();
    state->renderer->drawPoints(x, y, 0xffffffff);
    
    state->renderer->drawNumber(state->fpsPosX, state->fpsPosY, (int)fps, 0xffffff00, state->height/300);
    
    ANativeWindow_unlockAndPost(state->app->window);
}

extern "C" {
void android_main(struct android_app* app) {
    AppState state = {0};
    app->userData = &state;
    app->onAppCmd = handle_cmd;
    
    state.app = app;
    
    float counter = 0;
    uint64_t frameCount = 0;
    float dt=0.0f;
    float fps=0.0f;
    
    while (1) {
        int events;
        struct android_poll_source* source;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        while (ALooper_pollOnce(0, NULL, &events,
               (void**)&source) >= 0) {

            if (source)
                source->process(app, source);

            if (app->destroyRequested) {
                delete state.simulation;
                delete state.renderer;
                return;
            }
        }
        
        on_frame(&state, dt, fps);
        
        // FPS
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> duration = end - start;
        dt = duration.count();
        
        counter += dt;
        frameCount++;
        
        if (counter >= 1.0f) {
            fps = frameCount / counter;
            counter = 0;
            frameCount = 0;
        }
    }
}
}
