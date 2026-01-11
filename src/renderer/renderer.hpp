#pragma once

#include <print>
#include <span>

class Renderer {
private:
    uint32_t width;
    uint32_t height;
    
    uint32_t* buffer;

public:
    Renderer();
    void setBuffer(void* buffer, int width, int height);
    
    void drawPoint(int x, int y, uint32_t color);
    void drawPoints(std::span<const float> x, std::span<const float> y, uint32_t color);
    
    void drawDigit(int x, int y, int digit, uint32_t color, int scale=1);
    void drawNumber(int x, int y, int number, uint32_t color, int scale=1);
};

const uint8_t digits[10][7] = {
    {0b01110,0b10001,0b10011,0b10101,0b11001,0b10001,0b01110}, // 0
    {0b00100,0b01100,0b00100,0b00100,0b00100,0b00100,0b01110}, // 1
    {0b01110,0b10001,0b00001,0b00010,0b00100,0b01000,0b11111}, // 2
    {0b01110,0b10001,0b00001,0b00110,0b00001,0b10001,0b01110}, // 3
    {0b00010,0b00110,0b01010,0b10010,0b11111,0b00010,0b00010}, // 4
    {0b11111,0b10000,0b11110,0b00001,0b00001,0b10001,0b01110}, // 5
    {0b00110,0b01000,0b10000,0b11110,0b10001,0b10001,0b01110}, // 6
    {0b11111,0b00001,0b00010,0b00100,0b01000,0b01000,0b01000}, // 7
    {0b01110,0b10001,0b10001,0b01110,0b10001,0b10001,0b01110}, // 8
    {0b01110,0b10001,0b10001,0b01111,0b00001,0b00010,0b01100}  // 9
};
