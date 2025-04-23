#include "common.h"

#include <Arduino.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Arduino_JSON.h>

extern bool keep_on;

RTC_DATA_ATTR bool WIFI_CONFIGURED;
RTC_DATA_ATTR uint32_t lastIPAddress;
RTC_DATA_ATTR char lastSSID[30];

bool _connectWiFi() {
  // 配置了
  if (strlen(appConfig.wifiName) && strlen(appConfig.wifiPassword)) {
    if (WL_CONNECT_FAILED == WiFi.begin(appConfig.wifiName, appConfig.wifiPassword)) { // WiFi not setup, you can also use hard coded credentials with WiFi.begin(SSID,PASS);
      Serial.println("wifi begin error1");
      if (WL_CONNECT_FAILED == WiFi.begin()) { // WiFi not setup, you can also use hard coded credentials with WiFi.begin(SSID,PASS);
        Serial.println("wifi begin error2");
        return false;
      } else {
        Serial.println("wifi begin ok2");
      }
    } else {
      Serial.println("wifi begin ok1");
    }
  }
  // 没有配置, 则用ap配置的
  else {
    if (WL_CONNECT_FAILED == WiFi.begin()) { // WiFi not setup, you can also use hard coded credentials with WiFi.begin(SSID,PASS);
      Serial.println("wifi begin error0");
      return false;
    } else {
      Serial.println("wifi begin ok0");
    }
  }


  if (WL_CONNECTED == WiFi.waitForConnectResult()) { // attempt to connect for 10s
    lastIPAddress = WiFi.localIP();
    WiFi.SSID().toCharArray(lastSSID, 30);
    WIFI_CONFIGURED = true;

    Serial.println("connection ok");
    Serial.println(WiFi.SSID());
  } else { // connection failed, time out
    Serial.println("connection failed, time out");
    WIFI_CONFIGURED = false;
    // turn off radios
    WiFi.mode(WIFI_OFF);
    btStop();
  }
  return WIFI_CONFIGURED;
}

bool connectWiFi() {
  return _connectWiFi();
}

void _configModeCallback(WiFiManager *myWiFiManager) {
  console_log(50, "Connect to");
  console_log(50, "SSID: %s", WIFI_AP_SSID);
  console_log(50, "IP: %s", WiFi.softAPIP().toString().c_str());
	console_log(50, "MAC address:");
	console_log(50, WiFi.softAPmacAddress().c_str());
}

void setupWifi() {
  animation_reset_offsetY(); // 重置偏移量, 不然offsetY会导致超出屏幕
  OLED_ClearScreenBuffer();
  console_init(); // 初始化console, 重置从第一行开始
  
  WiFiManager wifiManager;
  wifiManager.resetSettings();
  wifiManager.setTimeout(WIFI_AP_TIMEOUT);
  wifiManager.setAPCallback(_configModeCallback);
  if (!wifiManager.autoConnect(WIFI_AP_SSID)) { // WiFi setup failed
    console_log(50, "Setup failed &");
    console_log(50, "timed out!");
  } else {
    console_log(50, "Connected to:");
    console_log(50, WiFi.SSID().c_str());
		console_log(50, "Local IP:");
		console_log(50, WiFi.localIP().toString().c_str());
    lastIPAddress = WiFi.localIP();
    WiFi.SSID().toCharArray(lastSSID, 30);

    // 保存起来
    strlcpy(appConfig.wifiName, WiFi.SSID().c_str(), sizeof(appConfig.wifiName));
    strlcpy(appConfig.wifiPassword, WiFi.psk().c_str(), sizeof(appConfig.wifiName));
    appconfig_save();
  }
  // turn off radios
  WiFi.mode(WIFI_OFF);
}

static bool btnExit()
{
  display_update_enable();
  keep_on = false;
  back_to_watchface();
  return true;
}

void showSetupWifi(void)
{
  // display_setDrawFunc(draw);
  display_setDrawFunc(NULL);
  buttons_setFuncs(btnExit, btnExit, btnExit);
  showMeThenRun(NULL);
  keep_on = true;

  display_update_disable();
  setupWifi();
}