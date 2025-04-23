#include "common.h"

#include <WiFiManager.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

extern bool keep_on;
extern WatchRTC RTC;
extern timeDate_s timeDate;

static bool wifiConnecting;
static bool wifiConnected;
static bool syncNTPing;
static bool syncNTPOk;

static bool btnExit()
{
  display_update_enable();
  keep_on = false;
  back_to_watchface();
  return true;
}

bool syncNTP(long gmt)
{
  String ntpServer = NTP_SERVER;
  // NTP sync - call after connecting to
  // WiFi and remember to turn it back off
  WiFiUDP ntpUDP;
  NTPClient timeClient(ntpUDP, ntpServer.c_str(), gmt);
  timeClient.begin();
  if (!timeClient.forceUpdate())
  {
    return false; // NTP sync failed
  }
  tmElements_t tm;
  breakTime((time_t)timeClient.getEpochTime(), tm);

  RTC.set(tm);
  RTC.readDatetime(timeDate);
  // 设置好时间则重新设置闹钟
  alarm_updateNextAlarm();

  return true;
}

static display_t draw()
{
  uint8_t y = 0;
  draw_string("Syncing NTP... ", false, 0, y);

  char buf[24];
  sprintf(buf, "GMT offset: %d", GMT_OFFSET_SEC);
  draw_string((char *)buf, false, 0, y += 9);

  if (wifiConnected)
  {
    draw_string("Wifi OK", false, 0, y += 9);
    if (syncNTPOk)
    {
      draw_string("NTP Sync OK", false, 0, y += 9);
      draw_string("Current Time:", false, 0, y += 9);
      RTC.readDatetime(timeDate);
      sprintf(buf, "%2d-%02d-%02d %02d:%02d:%02d", timeDate.date.year, timeDate.date.month + 1, timeDate.date.date, timeDate.time.hour, timeDate.time.mins, timeDate.time.secs);
      draw_string(buf, false, 0, y += 9);
    }
    else if (!syncNTPing)
    {
      draw_string("NTP Sync Failed", false, 0, y += 9);
    }
  }
  else if (!wifiConnecting)
  {
    draw_string("WiFi Not Configured", false, 0, y += 9);
  }

  return DISPLAY_BUSY;
}

void drawNTPStepByStep()
{
  animation_reset_offsetY(); // 重置偏移量, 不然offsetY会导致超出屏幕
  OLED_ClearScreenBuffer();
  console_init(); // 初始化console, 重置从第一行开始
  console_log(50, "Syncing NTP...");
  if (connectWiFi())
  {
    console_log(50, "Wifi OK");
    if (syncNTP(GMT_OFFSET_SEC))
    {
      console_log(50, "NTP Sync OK");
      console_log(50, "Current Time:");
      RTC.readDatetime(timeDate);
      console_log(50, "%2d-%02d-%02d %02d:%02d:%02d", timeDate.date.year, timeDate.date.month + 1, timeDate.date.date, timeDate.time.hour, timeDate.time.mins, timeDate.time.secs);
    }
    else
    {
      console_log(50, "NTP Sync Failed");
    }
  }
  else
  {
    console_log(50, "WiFi Not Configured");
  }
}

static bool retrySyncNtp()
{
  display_update_disable();
  drawNTPStepByStep();
  return true;
}

void showSyncNTP(void)
{
  // display_setDrawFunc(draw);
  display_setDrawFunc(NULL);
  buttons_setFuncs(retrySyncNtp, btnExit, retrySyncNtp);
  showMeThenRun(NULL);
  keep_on = true;

  display_update_disable();
  drawNTPStepByStep();

  // wifiConnecting = true;
  // wifiConnected = connectWiFi();
  // wifiConnecting = false;
  // if (wifiConnected) {
  //   syncNTPing = true;
  //   syncNTPOk = syncNTP(GMT_OFFSET_SEC);
  //   syncNTPing = false;
  // }
}