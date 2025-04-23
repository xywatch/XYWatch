/*
 * Project: N|Watch
 * Author: Zak Kemble, contact@zakkemble.co.uk
 * Copyright: (C) 2012 by Zak Kemble
 * License: GNU GPL v2 (see License.txt)
 * Web: http://blog.zakkemble.co.uk/diy-digital-wristwatch/
 */

#include "common.h"

#define NOALARM UCHAR_MAX

static draw_f oldDrawFunc;
static button_f oldBtn1Func;
static button_f oldBtn2Func;
static button_f oldBtn3Func;

RTC_DATA_ATTR byte nextAlarmIndex;
RTC_DATA_ATTR byte nextAlarmDay; // 下一个闹钟的星期, 0-6, 0:周一, 6:周日
RTC_DATA_ATTR bool isAlarmTriggered; // 

// 所有闹钟
RTC_DATA_ATTR alarm_s eepAlarms[ALARM_COUNT] EEMEM = {
    {12, 28, 255}, // 22:45:00, 127 = 1111111, 表示星期1,2,3,4,5,6,7, 255=1(开启) 111111(周二)1(周一), 表示所有星期且开启
    {12, 29, 255}, 
    {7, 45, 63},  // 63 = 111111, 表示星期1,2,3,4,5,6
    {9, 4, 0}, 
    {3, 1, 7} // 7 = 111, 表示星期1,2,3
};
RTC_DATA_ATTR bool isAlarmInited = false;
static bool need_updateAlarm_in_nextLoop = false;

static bool isAlarmTimeReached(void);
static void getNextAlarm(void);
static uint toMinutes(byte, byte, byte);
static bool stopAlarm(void);
static display_t draw(void);

extern bool keep_on;

bool alarm_is_enabled()
{
    u8 i;

    for (i = 0; i < ALARM_COUNT; i++)
    {
        if (eepAlarms[i].enabled == 1)
        {
            return 1;
        }
    }

    return 0;
}

// 初始化闹钟, 只执行一次, 在setup中调用
// 即使唤醒后, 也不会重新初始化
// 因为可能是因为RTC唤醒, 如果再执行这里, 闹钟就会变成下一个闹钟
void alarm_init()
{
    if (!isAlarmInited) {
        isAlarmInited = true;
        appconfig_init_alarm();
        getNextAlarm();
    }
}

void alarm_reset()
{
    memset(&eepAlarms, 0x00, ALARM_COUNT * sizeof(alarm_s));
}

void alarm_get(byte num, alarm_s *alarm)
{
    memcpy(alarm, &eepAlarms[num], sizeof(alarm_s));
}

bool alarm_getNext(alarm_s *alarm)
{
    if (nextAlarmIndex == NOALARM)
    {
        return false;
    }

    alarm_get(nextAlarmIndex, alarm);
    return true;
}

byte alarm_getNextDay()
{
    return nextAlarmDay;
}

void alarm_save(byte num, alarm_s *alarm)
{
    //	eeprom_update_block(alarm, &eepAlarms[num], sizeof(alarm_s));
    memcpy(&eepAlarms[num], alarm, sizeof(alarm_s));

    appconfig_save_alarm();
    
    // eepAlarms[num]=*alarm;
    getNextAlarm();
}

extern const uint32_t STAY[];

// 响应闹钟
// main.c 循环调用
void alarm_update()
{
    bool wasTriggered = isAlarmTriggered; // 之前为false
    bool alarmNow = isAlarmTimeReached(); // 调用这个方法后, isAlarmTriggered 会变为true

    // Serial.printf("alarm_update isAlarmTriggered: %d, alarmNow: %d\n", isAlarmTriggered, alarmNow);

    if (isAlarmTriggered) // 0秒闹钟触发
    {
        if (alarmNow) // 到达闹钟时间 1分钟内
        {
            if (!wasTriggered && isAlarmTriggered) // 闹钟第一次触发
            {
                keep_on = true;
                need_updateAlarm_in_nextLoop = false; // 下一次loop时不要getNextAlarm， 应该在stop时再getNextAlarm
                // Serial.printf("alarm_update isAlarmTriggered: %d, alarmNow: %d\n", isAlarmTriggered, alarmNow);
                oldDrawFunc = display_setDrawFunc(draw);
                oldBtn1Func = buttons_setFunc(BTN_1, NULL);
                oldBtn2Func = buttons_setFunc(BTN_2, stopAlarm);
                oldBtn3Func = buttons_setFunc(BTN_3, NULL);
                tune_play(STAY, VOL_ALARM, PRIO_ALARM);
            }
        }
        else  // 未到达闹钟时间 或 超过1分钟 就停止闹钟
        {
            stopAlarm();
        }
    }

    // 重新获取下一个alarm
    // 这里更新了alarm导致一下就stopAlarm了
    if (need_updateAlarm_in_nextLoop) {
        need_updateAlarm_in_nextLoop = false;
        getNextAlarm();
    }
}

// RTC中断时调用, 在下一个loop等alarm_update后再更新
// 因为RTC中断可能是由于整点报时, 所以这个中断后需要更新alarm
// 但也有可能真的是alarm, 所以需要等alarm_update后再更新
void alarm_need_updateAlarm_in_nextLoop () {
    need_updateAlarm_in_nextLoop = true;
}

void alarm_updateNextAlarm()
{
    getNextAlarm();
}

// 判断是否到达闹钟时间
static bool isAlarmTimeReached()
{
    alarm_s nextAlarm;

    // Make sure we're in 24 hour mode
    time_s time;
    time.hour = timeDate.time.hour;
    time.ampm = timeDate.time.ampm;
    time_timeMode(&time, TIMEMODE_24HR);

    if (alarm_getNext(&nextAlarm) && alarm_dayEnabled(nextAlarm.days, timeDate.date.day) && nextAlarm.hour == time.hour && nextAlarm.min == timeDate.time.mins)
    {
        if (timeDate.time.secs == 0)
        {
            // 只在秒为0时触发
            isAlarmTriggered = true;
        }

        // 到达闹钟时间, 1分钟内都算
        return true;
    }

    return false;
}

extern WatchRTC RTC;

// 设置了alarm不触发是怎么回事
// 偶尔不触发
void rtc_set_pcf8563_alarm (byte hour, byte min, day_t day) {
    byte pcf8563_day = dayTopPcf8563Day(day);
    Serial.printf("set alarm for %02d:%02d, nextAlarmDay: %d -> pcf8563_day: %d\n", hour, min, day, pcf8563_day);
    // delay(100);
    RTC.setAlarm(min, hour, pcf8563_day);
    RTC.printAlarm();
}

void rtc_set_alarm(void)
{
    // PCF8563只能设置一个闹钟
    // 要实现整点报时, 需要将下一个整点报时的时间设置到RTC中
    // 比如现在是12:01, 下一个整点报时是13:00, 则将13:00设置到RTC中
    // 得到下一个整点报时的时间
    timeDate_s nextZhengdian;
    time_getNextZhengdiao(&nextZhengdian);
    Serial.printf("nextZhengdian %02d:%02d, day:%d\n", nextZhengdian.time.hour, nextZhengdian.time.mins, nextZhengdian.date.day);

    alarm_s alarm;
    if (!alarm_getNext(&alarm))
    {
        // 清空闹钟
        // RTC.clearAlarm();

        // 没有闹钟, 则直接用下一个整点
        Serial.printf("没有闹钟, 则直接用下一个整点\n");
        rtc_set_pcf8563_alarm(nextZhengdian.time.hour, nextZhengdian.time.mins, nextZhengdian.date.day);
        return;
    }
    Serial.printf("nextAlarm %02d:%02d, day:%d\n", alarm.hour, alarm.min, nextAlarmDay);

    // 有设置闹钟, 则判断当前闹钟是否比整点早, 如果早, 则用闹钟的, 否则用整点的
    time_s timeNow;
    timeNow.hour = timeDate.time.hour;
    timeNow.ampm = timeDate.time.ampm;
    time_timeMode(&timeNow, TIMEMODE_24HR);

    // Now in minutes from start of week
    uint now = toMinutes(timeNow.hour, timeDate.time.mins + 1, timeDate.date.day);
    uint nextZhengdianMins = toMinutes(nextZhengdian.time.hour, nextZhengdian.time.mins + 1, nextZhengdian.date.day);
    uint nextAlarmMins = toMinutes(alarm.hour, alarm.min + 1, nextAlarmDay);

    // 整点比下一个闹钟早, 且比当前时间晚
    // now zd alarm
    if (now < nextZhengdianMins && nextZhengdianMins < nextAlarmMins)
    {
        Serial.println("now zd alarm");
        rtc_set_pcf8563_alarm(nextZhengdian.time.hour, nextZhengdian.time.mins, nextZhengdian.date.day);
        return;
    }

    // alarm now zd
    if (nextAlarmMins < now && now < nextZhengdianMins)
    {
        Serial.println("alarm now zd");
        rtc_set_pcf8563_alarm(nextZhengdian.time.hour, nextZhengdian.time.mins, nextZhengdian.date.day);
        return;
    }

    // zd alarm now
    if (nextZhengdianMins < nextAlarmMins && nextAlarmMins < now)
    {
        Serial.println("zd alarm now");
        rtc_set_pcf8563_alarm(nextZhengdian.time.hour, nextZhengdian.time.mins, nextZhengdian.date.day);
        return;
    }

    Serial.println("use alarm");
    rtc_set_pcf8563_alarm(alarm.hour, alarm.min, (day_t)nextAlarmDay);
}

// This func needs to be ran when an alarm has changed, time has changed or an active alarm has been turned off
static void getNextAlarm()
{
    byte next = NOALARM;
    uint nextTime = (uint)UINT_MAX;

    // Make sure time is in 24 hour mode
    time_s timeNow;
    timeNow.hour = timeDate.time.hour;
    timeNow.ampm = timeDate.time.ampm;
    time_timeMode(&timeNow, TIMEMODE_24HR);

    // Now in minutes from start of week
    uint now = toMinutes(timeNow.hour, timeDate.time.mins + 1, timeDate.date.day);

    Serial.printf("getNextAlarm now: %d-%d-%d %d:%d; wday:%d\n", timeDate.date.year, timeDate.date.month, timeDate.date.date, timeDate.time.hour, timeDate.time.mins, timeDate.date.day);

    // Loop through alarms
    LOOPR(ALARM_COUNT, i)
    {
        // Get alarm data
        alarm_s alarm;
        alarm_get(i, &alarm);

        // Not enabled
        if (!alarm.enabled)
        {
            continue;
        }

        // Loop through days 0-6 0-6, 星期, 0:周一, 6:周日
        LOOPR(7, d)
        {
            // Day not enabled
            if (!alarm_dayEnabled(alarm.days, d))
            {
                continue;
            }

            // Alarm time in minutes from start of week
            uint alarmTime = toMinutes(alarm.hour, alarm.min, d);

            // Minutes to alarm
            int timeTo = alarmTime - now;

            // Negative result, must mean alarm time is earlier in the week than now, add a weeks time
            // 比今天还早, 证明是下一周了, 要加上一周的分钟数
            if (timeTo < 0)
            {
                timeTo += ((6 * 1440) + (23 * 60) + 59); // 10079
            }

            // Is minutes to alarm less than the last minutes to alarm?
            // 比较是否是最近的一个日期
            if ((uint)timeTo < nextTime)
            {
                // This is our next alarm
                nextTime = timeTo;
                next = i;
                nextAlarmDay = d;
            }
        }
    }

    // Set next alarm
    nextAlarmIndex = next;

    rtc_set_alarm(); // RTC存入闹钟
}

static uint toMinutes(byte hours, byte mins, byte dow)
{
    uint total = mins;
    total += hours * 60;
    total += dow * 1440;
    return total;
}

static bool stopAlarm()
{
    getNextAlarm();
    display_setDrawFunc(oldDrawFunc);
    buttons_setFuncs(oldBtn1Func, oldBtn2Func, oldBtn3Func);
    //	oled_setInvert(appConfig.invert);
    //	pwrmgr_setState(PWR_ACTIVE_ALARM, PWR_STATE_NONE);
    tune_stop(PRIO_ALARM);
    isAlarmTriggered = false;

    Serial.println("stop alarm");
    keep_on = false;
    return true;
}

static display_t draw()
{
    draw_bitmap(16, 16, menu_alarm, 32, 32, NOINVERT, 0);

    // Draw time
    draw_string(time_timeStr(), NOINVERT, 79, 20);

    // Draw day
    char buff[BUFFSIZE_STR_DAYS];
    strcpy(buff, days[timeDate.date.day]);
    draw_string(buff, false, 86, 36);

    return DISPLAY_DONE;
}
