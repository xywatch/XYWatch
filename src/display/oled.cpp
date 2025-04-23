#include "common.h"
#include "Wire.h"

//-------------------------------------------------------------------------
/**************************************************************************/
// 坐标说明
/*														 x(0~127)
                                                 ------------------>
                                                |
                                                |
                                                |y(0~63)
                                                |
                                                |
                                                v

[x, y] => [x + y*128]
*/
u8 oledBuffer[FRAME_BUFFER_SIZE]; // 128*8

// 批量发送数据，减少I2C传输次数
void sendDataBatch(const uint8_t *data, size_t len)
{
    Wire.beginTransmission(OLED_ADDR);
    Wire.write(OLED_DATA);
    Wire.write(data, len);
    Wire.endTransmission();
}

void sendData(uint8_t data)
{
    Wire.beginTransmission(OLED_ADDR);
    Wire.write(OLED_DATA);
    Wire.write(data);
    Wire.endTransmission();
}

void sendCommand(uint8_t cmd)
{
    Wire.beginTransmission(OLED_ADDR);
    Wire.write(OLED_CMD);
    Wire.write(cmd);
    Wire.endTransmission();
}

void OLED_Clear(void) // 清屏
{
    unsigned char m, n;

    for (m = 0; m < 8; m++)
    {
        sendCommand(0xb0 + m); // page0-page1
        sendCommand(0x00);     // low column start address
        sendCommand(0x10);     // high column start address

        for (n = 0; n < 128; n++)
        {
            sendData(0x00);
        }
    }
}

void OLED_ON(void)
{
    sendCommand(0X8D); // 设置电荷泵
    sendCommand(0X14); // 开启电荷泵
    sendCommand(0XAF); // OLED唤醒
}

void OLED_OFF(void)
{
    sendCommand(0X8D); // 设置电荷泵
    sendCommand(0X10); // 关闭电荷泵
    sendCommand(0XAE); // OLED休眠
}

void driver_init()
{
    // Wire.begin();
    // Wire.setClock(800000); // 设置I2C时钟频率为800KHz
    // delay(100);

    sendCommand(0xAE); // display off
    sendCommand(0x20); // Set Memory Addressing Mode
    sendCommand(0x00); // 水平寻址模式
    sendCommand(0xB0); // Set Page Start Address
    sendCommand(0xC8); // Set COM Output Scan Direction
    sendCommand(0x00); // set low column address
    sendCommand(0x10); // set high column address
    sendCommand(0x40); // set start line address
    sendCommand(0x81); // set contrast control register
    sendCommand(0xFF); // 最大亮度
    sendCommand(0xA1); // set segment re-map
    sendCommand(0xA6); // normal display
    sendCommand(0xA8); // set multiplex ratio
    sendCommand(0x3F);
    sendCommand(0xA4); // output follows RAM content
    sendCommand(0xD3); // set display offset
    sendCommand(0x00); // no offset
    sendCommand(0xD5); // set display clock
    sendCommand(0xF0); // max frequency
    sendCommand(0xD9); // set pre-charge period
    sendCommand(0x22);
    sendCommand(0xDA); // set com pins
    sendCommand(0x12);
    sendCommand(0xDB); // set vcomh
    sendCommand(0x20);
    sendCommand(0x8D); // set DC-DC enable
    sendCommand(0x14);
    sendCommand(0xAF); // display on

    OLED_Clear();
}

void OLED_Init(void)
{
    driver_init();

    if (!appConfig.display180)
    {
        sendCommand(0xA1);
        sendCommand(0XC8);
    }
    else
    {
        sendCommand(0xA0);
        sendCommand(0xC0);
    }

    if (appConfig.brightness != 2) {
        if (appConfig.brightness > 3)
        {
            appConfig.brightness = 1;
        }

        if (appConfig.brightness == 0) {
            appConfig.brightness = 1;
        }
        sendCommand(0x81);            //--set contrast control register
        sendCommand(appConfig.brightness * 85); // 亮度调节 0x00~0xff
    }

}

// 优化后的OLED_Flush函数
void OLED_Flush(void)
{
    // 使用水平寻址模式，减少寻址命令
    sendCommand(0x20); // 设置寻址模式
    sendCommand(0x00); // 水平寻址模式

    // 设置列地址范围
    sendCommand(0x21); // 设置列地址
    sendCommand(0x00); // 起始地址
    sendCommand(0x7F); // 结束地址

    // 设置页地址范围
    sendCommand(0x22); // 设置页地址
    sendCommand(0x00); // 起始页
    sendCommand(0x07); // 结束页

    // 批量发送数据
    const int BATCH_SIZE = 32; // 每次发送32字节
    uint8_t *p = oledBuffer;

    for (int i = 0; i < FRAME_BUFFER_SIZE; i += BATCH_SIZE)
    {
        int len = min(BATCH_SIZE, FRAME_BUFFER_SIZE - i);
        if (appConfig.invert)
        {
            uint8_t tempBuffer[BATCH_SIZE];
            for (int j = 0; j < len; j++)
            {
                tempBuffer[j] = ~p[j];
            }
            sendDataBatch(tempBuffer, len);
        }
        else
        {
            sendDataBatch(p, len);
        }
        p += len;
    }
}

void OLED_ClearScreenBuffer(void)
{
    // Serial.println("OLED_ClearScreenBuffer");
    memset(&oledBuffer, 0x00, FRAME_BUFFER_SIZE);
}
