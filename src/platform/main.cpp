#include <termuxgui/termuxgui.h>

#include <print>
#include <chrono>
#include <cstring>
#include <cassert>

#include "renderer/renderer.hpp"
#include "core/simulation.hpp"

const uint32_t WIDTH = 1080 / 2;
const uint32_t HEIGHT = 2400 / 2;
const uint32_t FONT_SCALE = HEIGHT/300;

struct App {
    bool running;
    bool paused;
    
    tgui_connection c;
    tgui_activity a;
    
    tgui_view iv = -1;
    tgui_buffer* b;
};

bool initalize_app(App* app) {
    // Create connection
    if (tgui_connection_create(&app->c) != 0) {
        std::println("Error connection create");
        return false;
    }
    
    if (tgui_activity_create(app->c, &app->a, TGUI_ACTIVITY_NORMAL, NULL, false) != 0) {
        std::println("Error activity create");
        return false;
    }
    
    // The buffer
    app->b = new tgui_buffer{
        .width = WIDTH,
        .height = HEIGHT,
        .format = TGUI_BUFFER_FORMAT_ARGB8888
    };
    if (tgui_add_buffer(app->c, app->b) != 0) {
        std::println("Error addbuffer");
        return false;
    }
    
    app->running = true;
    app->paused = false;
    
    return true;
}

void poll_app_events(App* app) {
    tgui_event e;
    bool available;
    if (tgui_poll_event(app->c, &e, &available) != 0) {
        std::println("Error poll");
        app->running = false;
        return;
    }
    if (available) {
        if (e.type == TGUI_EVENT_CREATE) {
            if (app->iv == -1) {
                if (tgui_create_image_view(app->c, app->a, &app->iv, NULL, TGUI_VIS_VISIBLE, false) != 0) {
                    std::println("Error create imageview");
                    app->running = false;
                }
                if (tgui_set_buffer(app->c, app->a, app->iv, app->b) != 0) {
                    std::println("Error setbuffer");
                    app->running = false;
                }
            }
            app->paused = false;
        }
        if (e.type == TGUI_EVENT_PAUSE) {
            app->paused = true;
        }
        if (e.type == TGUI_EVENT_DESTROY) {
            app->running = false;
            tgui_connection_destroy(app->c);
        }

        tgui_event_destroy(&e);
    }
}
void update_image_view(App* app) {
    if (tgui_blit_buffer(app->c, app->b) != 0 ||
        tgui_refresh_image_view(app->c, app->a, app->iv) != 0) {
        std::println("Error buffer blit\n");
        app->running = false;
        return;
    }
}

int main() {
    App app{};
    Simulation* simulation = new Simulation(NUM_PARTICLES, WIDTH, HEIGHT);
    
    if (!initalize_app(&app)) {
        std::println("Could't initialize, closing");
        return 1;
    }
    
    Renderer* renderer = new Renderer;
    renderer->setBuffer(app.b->data, WIDTH, HEIGHT);
    
    int x=0;
    int y=0;
    
    float counter = 0;
    uint64_t frameCount = 0;
    float dt=0.0f;
    float fps=0.0f;
    
    while (app.running) {
        auto start = std::chrono::high_resolution_clock::now();
        
        poll_app_events(&app);
        if (!app.paused && app.iv != -1) {
            std::memset(app.b->data, 0, WIDTH*HEIGHT*4);
            
            simulation->update(dt);
            
            auto x=simulation->getX();
            auto y=simulation->getY();
            renderer->drawPoints(x, y, 0xffffffff);
            
            renderer->drawNumber(0, 0, (int)fps, 0xffffff00, FONT_SCALE);
            update_image_view(&app);
            
            
            // FPS
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> duration = end - start;
            dt = duration.count();
            
            counter += dt;
            frameCount++;
            
            if (counter >= 1.0f) {
                fps = frameCount / counter;
                std::println("{:.0f}", fps);
                counter = 0;
                frameCount = 0;
            }
        }
    }
    
    delete simulation;
    delete renderer;
    delete app.b;

    return 0;
}
