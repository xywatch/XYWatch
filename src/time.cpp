/*
 * Project: N|Watch
 * Author: Zak Kemble, contact@zakkemble.co.uk
 * Copyright: (C) 2013 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: http://blog.zakkemble.co.uk/diy-digital-wristwatch/
 */

#include "common.h"

#define SECONDS_IN_MIN 60
#define SECONDS_IN_HOUR (60 * SECONDS_IN_MIN)
#define SECONDS_IN_DAY (((uint32_t)24) * SECONDS_IN_HOUR)

#define FEB_LEAP_YEAR 29

extern WatchRTC RTC;

// 月份天数
static const byte monthDayCount[] PROGMEM = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

timeDate_s timeDate;
static uint32_t lastUpdateTime = 0;  // 记录上次更新时间

static void getRtcTime(void);

void time_init()
{
    time_wake();
}

void time_sleep()
{
}

void time_shutdown()
{
}

rtcwake_t time_wake()
{
    if (CONFIRM_BTN_KEY == 1) // 按下
    {
        return RTCWAKE_SYSTEM;
    }
    else
    {
        return RTCWAKE_NONE;
    }
}

void time_set(timeDate_s *newTimeDate)
{
    // 复制时间数据
    memcpy(&timeDate, newTimeDate, sizeof(timeDate_s));

    // 设置秒为0
    timeDate.time.secs = 0;

    // 转换24小时模式保存到RTC
    time_timeMode(&timeDate.time, TIMEMODE_24HR);

    Serial.printf("setDatetime: %d-%d-%d %d:%d:%d, day=%d\n", timeDate.date.year, timeDate.date.month, timeDate.date.date, timeDate.time.hour, timeDate.time.mins, timeDate.time.secs, timeDate.date.day);
    RTC.setDatetime(timeDate);

    // 设置好时间则重新设置闹钟
    alarm_updateNextAlarm();
}

bool time_isLeapYear(byte year)
{
    // Watch only supports years 2000 - 2099, so no need to do the full calculation

    return (year % 4 == 0);

    // uint fullYear = year + 2000;
    // return ((fullYear & 3) == 0 && ((fullYear % 25) != 0 || (fullYear & 15) == 0));
}

// Calculate day of week from year, month and date
// Using Zeller's congruence formula
// Parameters:
//   yy: year (0-99, representing 2000-2099)
//   m: month (0-11, where 0=January, 1=February, ..., 11=December)
//   d: day of month (1-31)
// Returns:
//   day of week (0=Monday, 1=Tuesday, ..., 6=Sunday)
day_t time_calcWeekDay(byte yy, month_t m, byte d)
{
    // 输入范围是0-11（对应1-12月）
    // 蔡勒公式使用的是1-14月
    // 调整月份和年份
    // 如果是1月或2月，按上一年的13月和14月计算
    u8 m2 = (u8)m;
    m2 += 1;
    if (m2 < 3) {
        m2 = (month_t)(m2 + 12);
        yy--;
    }
    
    int y = yy + 2000;  // 完整年份
    int k = y % 100;    // 年份后两位
    int j = y / 100;    // 年份前两位
    
    // 蔡勒公式
    int h = (d + ((13 * (m2 + 1)) / 5) + k + (k / 4) + (j / 4) - (2 * j)) % 7;
    
    // 确保结果为正数
    h = (h + 7) % 7;
    
    // 转换为我们需要的格式：0 = 周一，1 = 周二，...，6 = 周日
    // 蔡勒公式结果：0 = 周六，1 = 周日，2 = 周一，...，6 = 周五
    int dow = (h + 5) % 7;
    
    return (day_t)dow;
}

byte time_monthDayCount(month_t month, byte year)
{
    //	byte numDays = pgm_read_byte(&monthDayCount[month]);
    byte numDays = monthDayCount[month];

    if (month == MONTH_FEB && time_isLeapYear(year))
    {
        numDays = FEB_LEAP_YEAR;
    }

    return numDays;
}

char *time_timeStr()
{
    static char buff[BUFFSIZE_TIME_FORMAT_SMALL];
    sprintf_P(buff, PSTR(TIME_FORMAT_SMALL), timeDate.time.hour, timeDate.time.mins, timeDate.time.ampm);
    return buff;
}

// 转为时间mode, 12或24小时
/*
12小时制：将一天分为两个12小时周期，即上午（AM）和下午（PM）。
    AM（Ante Meridiem）：午夜12:00到中午11:59。
    PM（Post Meridiem）：中午12:00到午夜11:59。

12小时制的规则：
    一天被分为两个12小时周期：AM（午夜到中午）和PM（中午到午夜）。
    小时数从12开始，接着是1到11，没有0。
    AM周期：12:00 AM（午夜）→ 1:00 AM → … → 11:59 AM（上午）。
    PM周期：12:00 PM（中午）→ 1:00 PM → … → 11:59 PM（午夜前）。

24小时制：直接以0-24表示全天时间，无需区分AM/PM。例如，13:00即下午1点。
*/
void time_timeMode(time_s *time, timemode_t mode)
{
    byte hour = time->hour;

    // 转为12小时制
    if (mode == TIMEMODE_12HR)
    {
        if (time->ampm != CHAR_24) // Already 12hr
        {
            return;
        }
        // 之前是24小时, 现在转为12小时, 则13点变成下午1点
        else if (hour >= 12)
        {
            if (hour > 12)
            {
                hour -= 12;
            }

            time->ampm = CHAR_PM;
        }
        else
        {
            // 0点变成12点AM (12小时制没有0点)
            if (hour == 0)
            {
                hour = 12;
            }

            time->ampm = CHAR_AM;
        }
    }
    // 转为24小时制
    else
    {
        if (time->ampm == CHAR_AM && hour == 12) // Midnight 12AM = 00:00
        {
            hour = 0;
        }
        else if (time->ampm == CHAR_PM && hour < 12) // No change for 12PM (midday)
        {
            hour += 12;
        }

        time->ampm = CHAR_24;
    }

    time->hour = hour;
}

ulong lastMils = 0;
extern WatchRTC RTC;

// 更新时间
void time_update()
{
    uint32_t currentTime = millis();
    
    // 每1000ms（1秒）更新一次
    if (lastUpdateTime != 0 && currentTime - lastUpdateTime < 1000) {
        return;
    }
    lastUpdateTime = currentTime;

    RTC.readDatetime(timeDate);

    // RTC读出来的是24小时时间, 转为设置的12或24小时时间
    timeDate.time.ampm = CHAR_24;
	time_timeMode(&timeDate.time, appConfig.timeMode);

    // alarm_updateNextAlarm();

    // Serial.printf("time_update\n");
    // Serial.printf("%d-%d-%d %d:%d:%d\n", timeDate.date.year, timeDate.date.month, timeDate.date.date, timeDate.time.hour, timeDate.time.mins, timeDate.time.secs);

    // u8 rtcPin = digitalRead(RTC_INT_PIN);
    // Serial.printf("rtc pin: %d\n", rtcPin);

    // 整点报时
    if (timeDate.time.mins == 0 && timeDate.time.secs == 0)
    {
        tune_play(tuneHour, VOL_HOUR, PRIO_HOUR);
        Serial.println("整点报时");

        // 如果是在0点0分, 则需要reset step count
        if (timeDate.time.hour == 0) {
            resetStepCounter();
        }
    }

    // 时钟芯片错误, 需要重新配置
    if (timeDate.date.year == 0 || timeDate.date.date == 0)
    {
        RTC.config("2025:03:12:12:00:00");
    }
}

// 得到下一个整点报时的时间
void time_getNextZhengdiao(timeDate_s *timeDate2) {
    // 得到当前时间
    RTC.readDatetime(timeDate);

    // 这3个值可以不变, 不用管进位
    timeDate2->date.year = timeDate.date.year;
    timeDate2->date.month = timeDate.date.month;
    timeDate2->date.date = timeDate.date.date;

    timeDate2->time.hour = (timeDate.time.hour + 1) % 24; // 0-23
    timeDate2->time.mins = 0;
    timeDate2->time.secs = 0;

    // 第二天
    if (timeDate.time.hour == 23)
    {
        timeDate2->date.day = (day_t)((timeDate.date.day + 1) % 7); // 0-6
    }
    else
    {
        timeDate2->date.day = timeDate.date.day;
    }
}

/*
typedef struct
{
	day_t day; // 0-6, 星期, 0:周一, 6:周日
	byte date; // 1-31, 日期
	month_t month; // 0-11, 月份, 0:一月, 11:十二月
	byte year; // 0-99, 年份
} date_s;

typedef struct  { 
  uint8_t Second; 
  uint8_t Minute; 
  uint8_t Hour; 
  uint8_t Wday;   // 星期几 (1 = 星期日, 7 = 星期六)
  uint8_t Day; // 一个月中的第几天 (1 - 31)
  uint8_t Month; // 月份 1-12
  uint8_t Year;   // offset from 1970; 
} 	tmElements_t, TimeElements, *tmElementsPtr_t;
*/

// tmWday转为day_t
// tmWday 1-7, 1:星期日, 7:星期六
// day_t 0-6, 0:周一, 6:周日
day_t tmWdayToDay(uint8_t tmWday)
{
  if (tmWday == 1)
  {
    return DAY_SUN;
  }
  else if (tmWday == 7)
  {
    return DAY_SAT;
  }
  else
  {
    return (day_t)(tmWday - 2);
  }
}

// day_t转为tmWday
// day_t 0-6, 0:周一, 6:周日, 4:周五
// tmWday 1-7, 1:星期日, 7:星期六, 6:周五
uint8_t dayToTmWday(day_t day)
{
  if (day == DAY_SUN)
  {
    return 1;
  }
  else if (day == DAY_SAT)
  {
    return 7;
  }
  else
  {
    return (uint8_t)(day + 2);
  }
}

// PCF8563 的星期表示中，0表示周日, 6表示周六
uint8_t dayTopPcf8563Day(day_t day)
{
    return dayToTmWday(day)-1;
}

// timeDate month转为tmMonth
// timeDate month 0-11, 0:一月, 11:十二月
// tmMonth 1-12
uint8_t timeDateMonthToTmMonth(month_t month)
{
  return (uint8_t)(month + 1);
}

// tmMonth转为timeDate month
// tmMonth 1-12
// timeDate month 0-11, 0:一月, 11:十二月
month_t tmMonthToTimeDateMonth(uint8_t tmMonth)
{
  return (month_t)(tmMonth - 1);
}
