#ifndef __OLED_H
#define __OLED_H
#include "stdlib.h"

#define OLED_ADDR 0x3C
#define OLED_CMD 0x00
#define OLED_DATA 0x40
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

#define __SET_COL_START_ADDR() \
	do                         \
	{                          \
		sendCommand(0x00);     \
		sendCommand(0x10);     \
	} while (0)

void OLED_Init(void);  // 初始化													//关显示
void OLED_Clear(void); // 清屏
void OLED_Flush(void);
void OLED_ClearScreenBuffer(void);
void OLED_ON(void);
void OLED_OFF(void);
void sendCommand(uint8_t cmd);
void sendData(uint8_t data);
#endif
