#include "renderer.hpp"

Renderer::Renderer() {}

void Renderer::setBuffer(void* buffer, int width, int height) {
    this->buffer = (uint32_t*)buffer;
    this->width = width;
    this->height = height;
}

void Renderer::drawPoint(int x, int y, uint32_t color) {
    int index = y * this->width + x;
    if (index >= this->width * this->height)
        return;
    
    if (x < 0 || x > this->width)
        return;
    
    if (y < 0 || y > this->height)
        return;
    
    this->buffer[index] = color;
}

void Renderer::drawPoints(std::span<const float> x, std::span<const float> y, uint32_t color) {
    for (int i=0; i < y.size(); i++) {
        float px = x[i];
        float py = y[i];
        uint32_t* rowPtr = buffer + (int)py*this->width;
        if (px >= 0 && px < this->width && py >= 0 && py < this->height) {
            rowPtr[(int)px] = color;
        }
    }
}

void Renderer::drawDigit(int x, int y, int digit, uint32_t color, int scale) {
    if(digit < 0 || digit > 9) return;

    for(int row = 0; row < 7; ++row) {
        uint8_t line = digits[digit][row];
        for(int col = 0; col < 5; ++col) {
            if(line & (1 << (4 - col))) {
                for(int sy = 0; sy < scale; ++sy) {
                    int py = y + row * scale + sy;
                    uint32_t* rowPtr = buffer + py * width;
                    for(int sx = 0; sx < scale; ++sx) {
                        int px = x + col * scale + sx;
                        if(px >= 0 && px < this->width && py >= 0 && py < this->height) {
                            rowPtr[px] = color;
                        }
                    }
                }
            }
        }
    }
}

void Renderer::drawNumber(int x, int y, int number, uint32_t color, int scale) {
    if(number == 0) {
        drawDigit(x, y, 0, color, scale);
        return;
    }
    
    int digitsArray[10];
    int len = 0;
    while(number > 0) {
        digitsArray[len++] = number % 10;
        number /= 10;
    }
    
    for(int i = len-1; i >= 0; --i) {
        drawDigit(x, y, digitsArray[i], color, scale);
        x += (5 + 1) * scale;
    }
}