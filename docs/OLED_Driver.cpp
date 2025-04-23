#include "OLED_Driver.h"

void OLED_Driver::sendCommand(uint8_t cmd) {
    Wire.beginTransmission(OLED_ADDR);
    Wire.write(OLED_CMD);
    Wire.write(cmd);
    Wire.endTransmission();
}

void OLED_Driver::sendData(uint8_t data) {
    Wire.beginTransmission(OLED_ADDR);
    Wire.write(OLED_DATA);
    Wire.write(data);
    Wire.endTransmission();
}

void OLED_Driver::init() {
    Wire.begin();
    delay(100);
    
    // 初始化命令序列
    sendCommand(0xAE);    // 关闭显示
    sendCommand(0xD5);    // 设置显示时钟分频比/振荡器频率
    sendCommand(0x80);
    sendCommand(0xA8);    // 设置多路复用率
    sendCommand(0x3F);    // 63
    sendCommand(0xD3);    // 设置显示偏移
    sendCommand(0x00);    // 偏移0
    sendCommand(0x40);    // 设置显示开始行
    
    // // 尝试不同的显示方向组合
    // sendCommand(0xA0);    // 0xA0：正常左右方向，0xA1：左右反转
    // sendCommand(0xC0);    // 0xC0：正常上下方向，0xC8：上下反转
    
    sendCommand(0x81);    // 设置对比度命令
    sendCommand(0xFF);    // 最大对比度
    sendCommand(0xD9);    // 设置预充电周期
    sendCommand(0xF1);    
    sendCommand(0xDB);    // 设置VCOMH取消选择级别
    sendCommand(0x30);    
    sendCommand(0xA4);    // 全局显示开启;bit0:1,开启;0,关闭;(白屏/黑屏)
    sendCommand(0xA6);    // 设置显示方式;bit0:1,反相显示;0,正常显示
    sendCommand(0x8D);    // 设置充电泵
    sendCommand(0x14);    // 开启充电泵
    sendCommand(0xAF);    // 开启显示
    
    // 设置更高的对比度以提高刷新效果
    sendCommand(0x81);    // 对比度命令
    sendCommand(0xFF);    // 最大对比度
    
    // 关闭显示效果，可能会稍微提升速度
    sendCommand(0xA6);    // 正常显示（不反转）
    sendCommand(0xA4);    // 恢复到RAM内容显示
    
    // 使用水平寻址模式，减少寻址开销
    sendCommand(0x20);    // 设置寻址模式
    sendCommand(0x00);    // 水平寻址模式
    
    // 设置列地址范围
    sendCommand(0x21);    // 设置列地址
    sendCommand(0x00);    // 起始地址
    sendCommand(0x7F);    // 结束地址
    
    // 设置页地址范围
    sendCommand(0x22);    // 设置页地址
    sendCommand(0x00);    // 起始页
    sendCommand(0x07);    // 结束页
    
    // 清屏
    clear();
}

void OLED_Driver::clear() {
    // 清除所有8页
    for (int i = 0; i < 8; i++) {
        sendCommand(0xB0 + i);   // 设置页地址（0~7）
        sendCommand(0x00);       // 设置列地址低4位
        sendCommand(0x10);       // 设置列地址高4位
        // 清除这一页的所有列
        for (int j = 0; j < OLED_WIDTH; j++) {
            sendData(0x00);
        }
    }
}

void OLED_Driver::showString() {
    // 每个字节代表一个垂直的8像素列
    static const uint8_t HELLO[] = {
        // H (8列)
        0x00, 0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x00,
        // E (8列)
        0x00, 0x7E, 0x60, 0x60, 0x7C, 0x60, 0x7E, 0x00,
        // L (8列)
        0x00, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00,
        // L (8列)
        0x00, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00,
        // O (8列)
        0x00, 0x3C, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00
    };

    // 显示5个字符，每个字符占8列
    for (int char_idx = 0; char_idx < 5; char_idx++) {
        // 设置每个字符的起始位置
        sendCommand(0xB0);    // 页0
        sendCommand(0x00 | ((char_idx * 8) & 0x0F));       // 列地址低4位
        sendCommand(0x10 | ((char_idx * 8) >> 4));         // 列地址高4位

        // 显示一个字符的8列数据
        for (int col = 0; col < 8; col++) {
            sendData(HELLO[char_idx * 8 + col]);
        }
    }
}

void OLED_Driver::testPattern() {
    // 在第0页画一条横线
    sendCommand(0xB0);    // 页0
    sendCommand(0x00);    // 列地址低4位
    sendCommand(0x10);    // 列地址高4位
    
    for (int i = 0; i < OLED_WIDTH; i++) {
        sendData(0xFF);   // 一行全亮
    }
}

// // 优化数据传输方式
// void OLED_Driver::sendData(const uint8_t* data, size_t length) {
//     Wire.beginTransmission(OLED_ADDR);
//     Wire.write(OLED_DATA);
//     Wire.write(data, length);  // 批量发送数据
//     Wire.endTransmission();
// }