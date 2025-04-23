#include "common.h"

void initLED()
{
    // 设置LED引脚为输出
    pinMode(LED_1, OUTPUT);
    pinMode(LED_2, OUTPUT);
}

static bool lastLEDStatus = LOW;
void blinkLED()
{
    lastLEDStatus = lastLEDStatus ? LOW : HIGH;
    digitalWrite(LED_1, lastLEDStatus);
    digitalWrite(LED_2, lastLEDStatus ? LOW : HIGH);
}

void turnOffLED()
{
    digitalWrite(LED_1, LOW);
    digitalWrite(LED_2, LOW);
}