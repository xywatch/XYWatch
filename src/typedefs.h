/*
 * Project: N|Watch
 * Author: Zak Kemble, contact@zakkemble.co.uk
 * Copyright: (C) 2013 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: http://blog.zakkemble.co.uk/diy-digital-wristwatch/
 */

#ifndef TYPEDEFS_H_
#define TYPEDEFS_H_

#include "config/config.h"
#include <stdbool.h>
#include <stdint.h>

typedef uint8_t byte;

typedef uint8_t u8;
// typedef uint16_t uint; // conflicting types for
// typedef uint32_t ulong; // conflicting types for

extern byte oledBuffer[];

typedef unsigned int millis8_t;
typedef unsigned int millis_t;

// bool from stdbool does extra stuff to make the value
// always either 0 or 1, which isn't needed most of the time.
// So here's BOOL which doesn't do that.
typedef uint8_t BOOL;

typedef uint32_t timestamp_t;

typedef enum
{
	DISPLAY_DONE, // 屏幕刷新完成
	DISPLAY_BUSY, // 屏幕刷新中
	//	DISPLAY_TOOFAST // 刷新太频繁
} display_t;

typedef enum
{
	MENU_TYPE_STR,
	MENU_TYPE_ICON
} menu_type_t;

typedef enum
{
	MONTH_JAN = 0,
	MONTH_FEB = 1,
	MONTH_MAR = 2,
	MONTH_APR = 3,
	MONTH_MAY = 4,
	MONTH_JUN = 5,
	MONTH_JUL = 6,
	MONTH_AUG = 7,
	MONTH_SEP = 8,
	MONTH_OCT = 9,
	MONTH_NOV = 10,
	MONTH_DEC = 11
} month_t;

typedef enum
{
	DAY_MON = 0,
	DAY_TUE = 1,
	DAY_WED = 2,
	DAY_THU = 3,
	DAY_FRI = 4,
	DAY_SAT = 5,
	DAY_SUN = 6,
} day_t;

// typedef struct
// { 
//   uint8_t Second; 
//   uint8_t Minute; 
//   uint8_t Hour; 
//   uint8_t Wday;   // day of week, sunday is day 1
//   uint8_t Day;
//   uint8_t Month; 
//   uint8_t Year;   // offset from 1970; 
// } tmElements_t;

typedef struct
{
	byte secs;	// 0-59
	byte mins;	// 0-59
	byte hour; // 0-23
	char ampm; // A, P, 或 ' '
} time_s;

typedef struct
{
	day_t day; // 0-6, 星期, 0:周一, 6:周日
	byte date; // 1-31, 日期
	month_t month; // 0-11, 月份, 0:一月, 11:十二月
	byte year; // 0-99, 年份
} date_s;

typedef struct
{
	time_s time;
	date_s date;
} timeDate_s;

typedef struct
{
	unsigned char hour;
	unsigned char min;
	float temp;
	unsigned char shidu;
	int height;
} HistoryData;

typedef enum
{
	TIMEMODE_24HR = 0,
	TIMEMODE_12HR = 1
} timemode_t;

/*
days 和 结构体共享同一个字节的内存空间
可以通过两种方式访问这个字节：
作为一个完整的字节 (days)
作为 8 个独立的位 (每个布尔值占 1 位)
如果设置 days = 0b01010101，那么：
days = 0x55;  // 二进制：01010101
// 此时：
mon = 1;      // 第1位
tues = 0;     // 第2位
wed = 1;      // 第3位
thurs = 0;    // 第4位
fri = 1;      // 第5位
sat = 0;      // 第6位
sun = 1;      // 第7位
enabled = 0;  // 第8位
*/
typedef struct
{
	byte hour;
	byte min;
	union
	{
		byte days;
		struct
		{ // get rid of these bitfields?
			bool mon : 1;
			bool tues : 1;
			bool wed : 1;
			bool thurs : 1;
			bool fri : 1;
			bool sat : 1;
			bool sun : 1;
			bool enabled : 1;
		};
	};
} alarm_s;

// Could use bitfields for the bools to save a few bytes of RAM and EEPROM, but uses an extra ~82 bytes of flash
typedef struct
{
	char wifiName[32];
	char wifiPassword[32];
	// byte sleepMode;
	// byte sleepBrightness;
	byte sleepTimeout;
	// byte brightness;
	bool invert;
#if COMPILE_ANIMATIONS
	bool animations;
#endif
	// byte clockface;
	bool display180;
	bool CTRL_LEDs;
	bool showFPS;
	bool tiltWrist;
	bool doubleTap;
	timemode_t timeMode;
	union
	{
		uint32_t volumes;
		struct
		{ // get rid of these bitfields?
			byte volUI;
			byte volAlarm;
			byte volHour;
			byte brightness;
		};
	};
} appconfig_s;

typedef display_t (*draw_f)(void);
typedef void (*display_f)(void);

// Function for buttons to call
// Return true to only call function once per press
// Return false to continually call function while the button is pressed

// 定义一个返回值为bool的函数指针
typedef bool (*button_f)(void);

typedef void (*menu_f)(void);
typedef void (*itemLoader_f)(byte);

typedef struct
{
	menu_f btn1; // make array
	menu_f btn2;
	menu_f btn3;
	draw_f draw;
	itemLoader_f loader;
} menuFuncs_t;

typedef struct
{
	byte selected;
	byte scroll;
	byte optionCount;
	bool isOpen;
	const char *title;
	menu_type_t menuType;
	menuFuncs_t func;
	menu_f prevMenu;
} menu_s;

typedef struct
{
	byte lastSelected;
	menu_f last;
} prev_menu_s;

typedef struct
{
	bool active;				  // 开关
	byte offsetY;				  // 反转Y
	void (*animOnComplete)(void); // 指向的函数
	bool goingOffScreen;		  // 是否离开屏幕
} anim_s;

typedef struct
{
	byte x;
	byte y;
	const byte *bitmap;
	byte width;
	byte height;
	//	byte foreColour;
	bool invert;
	byte offsetY;
} image_s;

#include <Math.h>
#include <TimeLib.h>
typedef struct weatherData {
  double_t temperature;
  int16_t weatherConditionCode;
  bool isMetric;
  // String weatherDescription; // 不能用String, 会在deep sleep丢失
  char weatherDescription[20];
  char city[20];
  bool external;
  tmElements_t sunrise;
  tmElements_t sunset;
  double_t minTemp; // 不能用float
  double_t maxTemp;
  bool ok;
} weatherData;

#endif /* TYPEDEFS_H_ */
