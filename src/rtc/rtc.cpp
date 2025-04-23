#include "rtc.h"

void WatchRTC::init()
{
}

void WatchRTC::config(
    String datetime)
{ // String datetime format is YYYY:MM:DD:HH:MM:SS
  _PCFConfig(datetime);
}

void WatchRTC::clearAlarm()
{
  rtc_pcf.clearAlarm(); // resets the alarm flag in the RTC
}

// 设置下一分钟触发中断
void WatchRTC::setNextMinuteAlarm()
{
  rtc_pcf.clearAlarm(); // resets the alarm flag in the RTC

  // 下一分钟触发中断
  int nextAlarmMinute = 0;
  nextAlarmMinute = rtc_pcf.getMinute();
  nextAlarmMinute =
      (nextAlarmMinute == 59)
          ? 0
          : (nextAlarmMinute + 1); // set alarm to trigger 1 minute from now
  rtc_pcf.setAlarm(nextAlarmMinute, 99, 99, 99);
}

// 设置闹钟
void WatchRTC::setAlarm(uint8_t minute, uint8_t hour, uint8_t wday)
{
  rtc_pcf.clearAlarm(); // resets the alarm flag in the RTC
  rtc_pcf.setAlarm(minute, hour, 99, wday);
}

void WatchRTC::readDatetime(timeDate_s &timeDate)
{
  tmElements_t tm;
  read(tm);
  timeDate.time.secs = tm.Second;
  timeDate.time.mins = tm.Minute;
  timeDate.time.hour = tm.Hour;
  timeDate.date.day = tmWdayToDay(tm.Wday);
  timeDate.date.date = tm.Day;
  timeDate.date.month = tmMonthToTimeDateMonth(tm.Month);
  timeDate.date.year = tmYearToY2k(tm.Year); // tm.Year是以1970年为基准的，需要转换为2000年为基准

  // Serial.printf("timeDate: %d-%d-%d %d:%d:%d, day=%d\n", timeDate.date.year, timeDate.date.month, timeDate.date.date, timeDate.time.hour, timeDate.time.mins, timeDate.time.secs, timeDate.date.day);
}

void WatchRTC::read(tmElements_t &tm)
{

  rtc_pcf.getDate();
  tm.Year = y2kYearToTm(rtc_pcf.getYear());
  tm.Month = rtc_pcf.getMonth();
  tm.Day = rtc_pcf.getDay();
  tm.Wday =
      rtc_pcf.getWeekday() + 1; // TimeLib & DS3231 has Wday range of 1-7, but
                                // PCF8563 stores day of week in 0-6 range
  tm.Hour = rtc_pcf.getHour();
  tm.Minute = rtc_pcf.getMinute();
  tm.Second = rtc_pcf.getSecond();
}

void WatchRTC::setDatetime(timeDate_s timeDate)
{
  // 将 timeDate 转为 tmElements_t
  tmElements_t tm;
  tm.Year = y2kYearToTm(timeDate.date.year);
  tm.Month = timeDateMonthToTmMonth(timeDate.date.month);
  tm.Day = timeDate.date.date;
  tm.Wday = dayToTmWday(timeDate.date.day);
  tm.Hour = timeDate.time.hour;
  tm.Minute = timeDate.time.mins;
  tm.Second = timeDate.time.secs;

  Serial.printf("setDatetime tm: %d-%d-%d %d:%d:%d, day=%d\n", tm.Year, tm.Month, tm.Day, tm.Hour, tm.Minute, tm.Second, tm.Wday);
  set(tm);
}

void WatchRTC::set(tmElements_t tm)
{
  time_t t = makeTime(tm); // make and break to calculate tm.Wday
  breakTime(t, tm);
  // day, weekday, month, century(1=1900, 0=2000), year(0-99)
  rtc_pcf.setDate(
      tm.Day, tm.Wday - 1, tm.Month, 0,
      tmYearToY2k(tm.Year)); // TimeLib & DS3231 has Wday range of 1-7, but
                             // PCF8563 stores day of week in 0-6 range
  // hr, min, sec
  rtc_pcf.setTime(tm.Hour, tm.Minute, tm.Second);

  // 重新设置时间则先清除闹钟
  clearAlarm();
}

uint8_t WatchRTC::temperature()
{
  return 255; // error
}

void WatchRTC::_PCFConfig(
    String datetime)
{ // String datetime is YYYY:MM:DD:HH:MM:SS
  if (datetime != "")
  {
    tmElements_t tm;
    tm.Year = CalendarYrToTm(_getValue(datetime, ':', 0).toInt()); // YYYY -
                                                                   // 1970
    tm.Month = _getValue(datetime, ':', 1).toInt();
    tm.Day = _getValue(datetime, ':', 2).toInt();
    tm.Hour = _getValue(datetime, ':', 3).toInt();
    tm.Minute = _getValue(datetime, ':', 4).toInt();
    tm.Second = _getValue(datetime, ':', 5).toInt();
    time_t t = makeTime(tm); // make and break to calculate tm.Wday
    breakTime(t, tm);
    // day, weekday, month, century(1=1900, 0=2000), year(0-99)
    rtc_pcf.setDate(
        tm.Day, tm.Wday - 1, tm.Month, 0,
        tmYearToY2k(tm.Year)); // TimeLib & DS3231 has Wday range of 1-7, but
                               // PCF8563 stores day of week in 0-6 range
    // hr, min, sec
    rtc_pcf.setTime(tm.Hour, tm.Minute, tm.Second);
  }

  // on POR event, PCF8563 sets month to 0, which will give an error since
  // months are 1-12
  clearAlarm();
}

String WatchRTC::_getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void WatchRTC::printAlarm()
{
  rtc_pcf.getAlarm(); // 从芯片获取数据
  Serial.printf("rtc_pcf alarm: %d:%d, weekDay: %d, date: %d\n", rtc_pcf.getAlarmHour(), rtc_pcf.getAlarmMinute(), rtc_pcf.getAlarmWeekday(), rtc_pcf.getAlarmDay());
}