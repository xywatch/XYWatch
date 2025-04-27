/*
 * Project: N|Watch
 * Author: Zak Kemble, contact@zakkemble.co.uk
 * Copyright: (C) 2013 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: http://blog.zakkemble.co.uk/diy-digital-wristwatch/
 */

#include "common.h"

extern BMA423 sensor;

void drawBattery();
void drawStep(byte);
void getBatteryAndOthers();

#define TIME_POS_X 0
#define TIME_POS_Y 17
#define TICKER_GAP 4

// 8 * 16 = 128

typedef struct
{
    byte x;
    byte y;
    const byte *bitmap;
    byte w;
    byte h;
    byte offsetY;
    byte val;
    byte maxVal;
    bool moving;
} tickerData_t;

static display_t draw(void);
static void drawDate(void);
static display_t ticker(void);
static void drawTickerNum(tickerData_t *);

byte seconds = 0;

// watch face
void watchface_normal()
{
    display_setDrawFunc(draw);
    // buttons_setFuncs(altitude_open, menu_select, my_menu_open2);
    buttons_setFuncs(showWeather, menu_select, my_menu_open2);
    showMeThenRun(NULL);
}

static uint32_t lastUpdateTime = 0;  // 记录上次更新时间

static bool animateIcon(bool active, byte* pos)
{
	byte y = *pos;
	if(active || (!active && y < FRAME_HEIGHT))
	{
		if(active && y > FRAME_HEIGHT - 8)
			y -= 1;
		else if(!active && y < FRAME_HEIGHT)
			y += 1;
		*pos = y;
		return true;
	}
	return false;
}

static display_t draw()
{
    // Draw date
    drawDate();

    // Draw time animated
    display_t busy;

    busy = ticker();

    uint32_t currentTime = millis();
    
    // 每1000ms（1秒）更新一次
    if (currentTime - lastUpdateTime >= 5000) {
        // Draw battery icon
        getBatteryAndOthers();
        lastUpdateTime = currentTime;
    }
    drawBattery();

    byte x = 35;

    static byte usbImagePos = FRAME_HEIGHT;
	static byte chargeImagePos = FRAME_HEIGHT;

    // USB是否已插入
    if(animateIcon(USB_CONNECTED(), &usbImagePos))
	{
		draw_bitmap(x, usbImagePos, usbIcon, 16, 8, NOINVERT, 0);
		x += 20;
	}
    // 是否在充电
    // if(animateIcon(CHARGING(), &chargeImagePos))
	// {
	// 	draw_bitmap(x, chargeImagePos, chargeIcon, 8, 8, NOINVERT, 0);
	// 	x += 12;
	// }

#if COMPILE_STOPWATCH
    // Stopwatch icon
    if (stopwatch_active())
    {
        draw_bitmap(x, FRAME_HEIGHT - 8, stopwatch, 8, 8, NOINVERT, 0);
        x += 12;
    }
#endif

    // Draw next alarm
	alarm_s nextAlarm;
	if(alarm_getNext(&nextAlarm))
	{
		time_s alarmTime;
		alarmTime.hour = nextAlarm.hour;
		alarmTime.mins = nextAlarm.min;
		alarmTime.ampm = CHAR_24;
		time_timeMode(&alarmTime, appConfig.timeMode);
		
		char buff[9];
		sprintf_P(buff, PSTR("%02hhu:%02hhu%c"), alarmTime.hour, alarmTime.mins, alarmTime.ampm);
		draw_string(buff, false, x, FRAME_HEIGHT - 8);

		x += (alarmTime.ampm == CHAR_24) ? 35 : 42;
		draw_bitmap(x, FRAME_HEIGHT - 8, dowImg[alarm_getNextDay()], 8, 8, NOINVERT, 0);
		x += 10;
	}

    // if (alarm_is_enabled())
    // {
    //     draw_bitmap(x, FRAME_HEIGHT - 8, smallFontAlarm, 8, 8, NOINVERT, 0);
    //     x += 12;
    // }

    drawStep(x);

    // 显示温度
    // weatherData currentWeather = getWeather();
    // if (currentWeather.ok)
    // {
    //     char temp[8];
    //     sprintf((char *)temp, "%0.1fC", currentWeather.temperature);
    //     draw_string(temp, NOINVERT, FRAME_WIDTH - 35, FRAME_HEIGHT - 8);
    // }

    // 湿度
    // sprintf((char *)temp, "%0.0f%%", 12); 
    // draw_string(temp, NOINVERT, FRAME_WIDTH - 60, FRAME_HEIGHT - 8);

    return busy;
}

static void drawDate()
{
    /*
    if (!timeDate.date.year) {
        // 为了能正常显示
        time_s time = {seconds, 12, 18, 'A'};
        date_s date = {DAY_THU, 12, MONTH_OCT, 22};

        timeDate.time = time;
        timeDate.date = date;
    }
    */

    // Get day string
    char day[BUFFSIZE_STR_DAYS];
    strcpy(day, days[timeDate.date.day]);

    // Get month string
    char month[BUFFSIZE_STR_MONTHS];
    strcpy(month, months[timeDate.date.month]);

    // Draw date
    char buff[BUFFSIZE_DATE_FORMAT];
    // sprintf_P(buff, PSTR(DATE_FORMAT), day, timeDate.date.date, month, timeDate.date.year);
    sprintf(buff, "%02d-%02d-%02d %s", timeDate.date.year, timeDate.date.month + 1, timeDate.date.date, day);

    u8 x = 0;
    // 显示温度
    weatherData currentWeather = getWeather();
    if (currentWeather.ok)
    {
        draw_string(buff, false, x, 0);
        char temp[8];
        sprintf((char *)temp, "%0.1fC", currentWeather.temperature);
        draw_string(temp, NOINVERT, FRAME_WIDTH - 35, 0);
    }
    else {
        draw_string_center(buff, false, x, 127, 0);
    }
}

static display_t ticker()
{
    static byte yPos;
    static byte yPos_secs;
    static bool moving = false;
    static bool moving2[5];

    /*
    if(milliseconds % 3600 > 1800) {
        seconds++;
        seconds = seconds % 60;
        timeDate.time.secs = seconds;
    }
    */

    static byte hour2;
    static byte mins;
    static byte secs;

    if (appConfig.animations)
    {
        if (timeDate.time.secs != secs)
        {
            // Serial.println("show...");
            // Serial.println(secs);

            yPos = 0;
            yPos_secs = 0;
            moving = true;

            moving2[0] = div10(timeDate.time.hour) != div10(hour2);
            moving2[1] = mod10(timeDate.time.hour) != mod10(hour2);
            moving2[2] = div10(timeDate.time.mins) != div10(mins);
            moving2[3] = mod10(timeDate.time.mins) != mod10(mins);
            moving2[4] = div10(timeDate.time.secs) != div10(secs);

            // moving2[3] = 1;

            // memcpy(&timeDateLast, &timeDate, sizeof(timeDate_s));
            hour2 = timeDate.time.hour;
            mins = timeDate.time.mins;
            secs = timeDate.time.secs;
        }

        // 之前height是 16 24
        // 改成了 16 32
        // TICKER_GAP 有什么用? 秒和时针的结束动画错开, 时晚4
        if (moving)
        {
            if (yPos <= 3)
            {
                yPos++;
            }
            else if (yPos <= 6)
            {
                yPos += 3;
            }
            else if (yPos <= 16)
            {
                yPos += 5;
            }
            else if (yPos <= MIDFONT_NUM_HEIGHT - 3)
            { // 22 前快后慢
                yPos += 3;
            }
            else if (yPos <= MIDFONT_NUM_HEIGHT + TICKER_GAP)
            { // 24 + TICKER_GAP, 如果还用24会卡顿
                yPos++;
            }

            if (yPos >= MIDFONT_NUM_HEIGHT + TICKER_GAP)
            {
                yPos = 255;
            }

            if (yPos_secs <= 1)
            {
                yPos_secs++;
            }
            else if (yPos_secs <= SMALLFONT_NUM_HEIGHT - 3)
            {
                yPos_secs += 3;
            }
            else if (yPos_secs <= SMALLFONT_NUM_HEIGHT + TICKER_GAP)
            {
                yPos_secs++;
            }

            if (yPos_secs >= SMALLFONT_NUM_HEIGHT + TICKER_GAP)
            {
                yPos_secs = 255;
            }

            if (yPos_secs > SMALLFONT_NUM_HEIGHT + TICKER_GAP && yPos > MIDFONT_NUM_HEIGHT + TICKER_GAP)
            {
                yPos = 0;
                yPos_secs = 0;
                moving = false;
                memset(moving2, false, sizeof(moving2));
            }
        }
    }
    else
    {
        yPos = 0;
        yPos_secs = 0;
        moving = false;
        memset(moving2, false, sizeof(moving2));
    }

    tickerData_t data;

    // Set new font data for hours and minutes
    data.y = TIME_POS_Y;
    // data.w = MIDFONT_WIDTH;
    // data.h = MIDFONT_HEIGHT;
    data.w = MIDFONT_NUM_WIDTH;
    data.h = MIDFONT_NUM_HEIGHT;
    // data.bitmap = (const byte*)&midFont;
    data.bitmap = (const byte *)&numFont16x32;
    data.offsetY = yPos;

    // Hours
    data.x = TIME_POS_X;
    data.val = div10(timeDate.time.hour);
    data.maxVal = 2;
    data.moving = moving2[0];
    drawTickerNum(&data);

    data.x += 16;
    data.val = mod10(timeDate.time.hour);
    data.maxVal = 9;
    data.moving = moving2[1];
    drawTickerNum(&data);

    data.x += 16;

    // Draw colon for half a second   画半秒的冒号
    // if(milliseconds % 3600 > 1800) { // 假装是半秒钟  30ms
    // draw_bitmap(TIME_POS_X + 46 + 2, TIME_POS_Y, colon, FONT_COLON_WIDTH, FONT_COLON_HEIGHT, NOINVERT, 0);
    draw_bitmap(data.x, TIME_POS_Y, numFont16x32[10], MIDFONT_NUM_WIDTH, MIDFONT_NUM_HEIGHT, NOINVERT, 0);
    //}

    // Minutes
    data.x += 16;
    data.val = div10(timeDate.time.mins);
    data.maxVal = 5;
    data.moving = moving2[2];
    drawTickerNum(&data);

    data.x += 16;
    data.val = mod10(timeDate.time.mins);
    data.maxVal = 9;
    data.moving = moving2[3];
    drawTickerNum(&data);
    data.x += 16;

    // Seconds
    data.y += 16;

    // if(milliseconds % 3600 > 1800) { // 假装是半秒钟  30ms
    if (millis() % 1000 >= 500)
    { // 0.5s 1秒分成两断, 后半秒显示, 前半秒隐藏
        // draw_bitmap(TIME_POS_X + 46 + 2, TIME_POS_Y, colon, FONT_COLON_WIDTH, FONT_COLON_HEIGHT, NOINVERT, 0);
        draw_bitmap(data.x, data.y, numFont16x16[10], SMALLFONT_NUM_WIDTH, SMALLFONT_NUM_HEIGHT, NOINVERT, 0);
    }

    data.x += 16;

    // data.bitmap = (const byte*)&small2Font;
    data.bitmap = (const byte *)&numFont16x16;
    // data.w = FONT_SMALL2_WIDTH;
    // data.h = FONT_SMALL2_HEIGHT;
    data.w = SMALLFONT_NUM_WIDTH;
    data.h = SMALLFONT_NUM_HEIGHT;
    data.offsetY = yPos_secs;
    data.val = div10(timeDate.time.secs);
    data.maxVal = 5;
    data.moving = moving2[4];
    drawTickerNum(&data);

    data.x += 16;
    data.val = mod10(timeDate.time.secs);
    data.maxVal = 9;
    data.moving = moving;
    drawTickerNum(&data);

    // Draw AM/PM character
    char tmp[2];
    tmp[0] = timeDate.time.ampm;
    tmp[1] = 0x00;
    draw_string(tmp, false, 104, 20);

    //	char buff[12];
    //	sprintf_P(buff, PSTR("%lu"), time_getTimestamp());
    //	draw_string(buff, false, 30, 50);

    return (moving ? DISPLAY_BUSY : DISPLAY_DONE);
}

static void drawTickerNum(tickerData_t *data)
{
    byte arraySize = (data->w * data->h) / 8;
    byte yPos = data->offsetY;
    const byte *bitmap = &data->bitmap[data->val * arraySize];
    byte x = data->x;
    byte y = data->y;

    if (!data->moving || yPos == 0 || yPos == 255)
    {
        draw_bitmap(x, y, bitmap, data->w, data->h, NOINVERT, 0);
    }
    else
    {
        byte prev = data->val - 1;

        if (prev == 255)
        {
            prev = data->maxVal;
        }

        draw_bitmap(x, y, bitmap, data->w, data->h, NOINVERT, yPos - data->h - TICKER_GAP);
        draw_bitmap(x, y, &data->bitmap[prev * arraySize], data->w, data->h, NOINVERT, yPos);
    }
}

// 获取电池电压
float getBatteryVoltage() {
  return analogReadMilliVolts(BATT_ADC_PIN) / 1000.0f * 2.0f;
}

RTC_DATA_ATTR float VBAT;
RTC_DATA_ATTR uint32_t stepCount;

void getBatteryAndOthers () {
    VBAT = getBatteryVoltage();
    stepCount = sensor.getCounter();
    // syncWeather();
}

// 绘制电池图标
void drawBattery()
{
    char ad[5];
    const byte *battIcon;

    // 根据电量百分比选择电池图标
    int8_t batteryLevel = 0;
    
    // 更细致的电压到电量的映射关系
    // 锂电池的电压与电量关系通常是非线性的
    // 满电约4.2V，空电约3.3V
    if (VBAT > 4.15)
    {
        batteryLevel = 99; // 满电
        battIcon = battIconFull;
    }
    else if (VBAT > 4.1)
    {
        batteryLevel = 95;
        battIcon = battIconFull;
    }
    else if (VBAT > 4.05)
    {
        batteryLevel = 90;
        battIcon = battIconFull;
    }
    else if (VBAT > 4.0)
    {
        batteryLevel = 85;
        battIcon = battIconFull;
    }
    else if (VBAT > 3.95)
    {
        batteryLevel = 80;
        battIcon = battIconFull;
    }
    else if (VBAT > 3.9)
    {
        batteryLevel = 70;
        battIcon = battIconHigh;
    }
    else if (VBAT > 3.85)
    {
        batteryLevel = 60;
        battIcon = battIconHigh;
    }
    else if (VBAT > 3.8)
    {
        batteryLevel = 50;
        battIcon = battIconHigh;
    }
    else if (VBAT > 3.75)
    {
        batteryLevel = 40;
        battIcon = battIconHigh;
    }
    else if (VBAT > 3.7)
    {
        batteryLevel = 30;
        battIcon = battIconLow;
    }
    else if (VBAT > 3.65)
    {
        batteryLevel = 25;
        battIcon = battIconLow;
    }
    else if (VBAT > 3.6)
    {
        batteryLevel = 20;
        battIcon = battIconLow;
    }
    else if (VBAT > 3.55)
    {
        batteryLevel = 15;
        battIcon = battIconLow;
    }
    else if (VBAT > 3.5)
    {
        batteryLevel = 10;
        battIcon = battIconLow;
    }
    else if (VBAT > 3.45)
    {
        batteryLevel = 5;
        battIcon = battIconLow;
    }
    else
    {
        batteryLevel = 0;
        battIcon = battIconEmpty;
    }

    // 如果在充电, 则绘制充电的图标
    if(CHARGING())
	{
		// draw_bitmap(4, FRAME_HEIGHT - 8, chargeIcon, 8, 8, NOINVERT, 0);
        draw_bitmap(0, FRAME_HEIGHT - 8, chargeIcon2, 16, 8, NOINVERT, 0);
	}
    else
    {
        // 绘制电池图标
        draw_bitmap(0, FRAME_HEIGHT - 8, battIcon, 16, 8, NOINVERT, 0);
    }

    // 显示电量百分比
    sprintf((char *)ad, "%d", batteryLevel);
    draw_string(ad, NOINVERT, 18, FRAME_HEIGHT - 8);

    // Serial.printf("VBAT: %f, bat: %d\n", VBAT, batteryLevel);
}

void drawStep(byte x) {
    char ad[5];
    // uint32_t stepCount = sensor.getCounter();
    sprintf((char *)ad, "%d", (int)stepCount);
    // draw_string(ad, NOINVERT, x, FRAME_HEIGHT - 8);

    // 靠右显示
    u8 len = 10;
    if (stepCount < 10) {
        len = 1;
    } else if (stepCount < 100) {
        len = 2;
    } else if (stepCount < 1000) {
        len = 3;
    } else if (stepCount < 10000) {
        len = 4;
    }
     else if (stepCount < 100000) {
        len = 5;
    }

    x = 127 - len * 7 - 10; // 每一个字5像素宽
    draw_bitmap(x, FRAME_HEIGHT - 8, step, 8, 8, NOINVERT, 0);
    x += 10;
    draw_string(ad, NOINVERT, x, FRAME_HEIGHT - 8);
}