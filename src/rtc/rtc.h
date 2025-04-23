#ifndef RTC_H
#define RTC_H

#include <TimeLib.h>        // https://github.com/PaulStoffregen/Time
#include <Rtc_Pcf8563.h>

#define RTC_PCF_ADDR    0x51
#define YEAR_OFFSET_PCF 2000

class WatchRTC {
public:
  Rtc_Pcf8563 rtc_pcf;

public:
  void init();
  void config(String datetime); // String datetime format is YYYY:MM:DD:HH:MM:SS
  void clearAlarm();
  void setNextMinuteAlarm();
  void read(tmElements_t &tm);
  void readDatetime(timeDate_s &timeDate);
  void set(tmElements_t tm);
  void setDatetime(timeDate_s timeDate);
  void setAlarm(uint8_t minute, uint8_t hour, uint8_t wday);
  uint8_t temperature();
  void printAlarm();

private:
  void _PCFConfig(String datetime);
  String _getValue(String data, char separator, int index);
};

#endif