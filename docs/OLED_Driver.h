#ifndef OLED_DRIVER_H
#define OLED_DRIVER_H

#include <Arduino.h>
#include <Wire.h>

class OLED_Driver {
private:
    static const uint8_t OLED_ADDR = 0x3C;
    static const uint8_t OLED_CMD = 0x00;
    static const uint8_t OLED_DATA = 0x40;
    static const uint8_t OLED_WIDTH = 128;
    static const uint8_t OLED_HEIGHT = 64;

    void sendCommand(uint8_t cmd);
    void sendData(uint8_t data);

public:
    void init();
    void clear();
    void showString();
    void testPattern();
};

#endif 