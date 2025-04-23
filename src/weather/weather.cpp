#include "common.h"

#include <WiFiManager.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Arduino_JSON.h>

extern bool keep_on;
extern WatchRTC RTC;
extern timeDate_s timeDate;

static bool weather_wifiConnecting;
static bool weather_wifiConnected;
static bool weather_syncing;
static bool weather_syncOK;

RTC_DATA_ATTR weatherData currentWeather;
RTC_DATA_ATTR uint16_t lastFetchWeatherMinute = 0;

static bool btnExit()
{
    keep_on = false;
    back_to_watchface();
    return true;
}

bool syncWeather()
{
    String cityID = CITY_ID;
    String lat = LAT;
    String lon = LON;
    String units = TEMP_UNIT;
    String lang = TEMP_LANG;
    String url = OPENWEATHERMAP_URL;
    String apiKey = OPENWEATHERMAP_APIKEY;
    uint8_t updateInterval = WEATHER_UPDATE_INTERVAL;
    
    currentWeather.isMetric = units == String("metric");

    RTC.readDatetime(timeDate);
    uint16_t curMinute = timeDate.date.date * 24 * 60 + timeDate.time.hour * 60 + timeDate.time.mins;
    Serial.println("_getWeatherData");
    // Serial.println(curMinute);
    // Serial.println(lastFetchWeatherMinute);
    // Serial.println(curMinute - lastFetchWeatherMinute);
    if (curMinute - lastFetchWeatherMinute >= updateInterval) // only update if WEATHER_UPDATE_INTERVAL has elapsed i.e. 30 minutes
    {
        if (connectWiFi())
        {
            weather_wifiConnected = true;
            Serial.println("WIFI connected");
            HTTPClient http;              // Use Weather API for live data if WiFi is connected
            http.setConnectTimeout(3000); // 3 second max timeout
            String weatherQueryURL = url;
            if (cityID != "")
            {
                weatherQueryURL.replace("{cityID}", cityID);
            }
            else
            {
                weatherQueryURL.replace("{lat}", lat);
                weatherQueryURL.replace("{lon}", lon);
            }
            weatherQueryURL.replace("{units}", units);
            weatherQueryURL.replace("{lang}", lang);
            weatherQueryURL.replace("{apiKey}", apiKey);
            http.begin(weatherQueryURL.c_str());
            int httpResponseCode = http.GET();
            if (httpResponseCode == 200)
            {
                String payload = http.getString();
                JSONVar responseObject = JSON.parse(payload);
                currentWeather.external = true;
                currentWeather.ok = true;

                // 只保留一位小数
                currentWeather.temperature = floor((double_t)responseObject["main"]["temp"] * 10) / 10;
                currentWeather.minTemp = floor((double_t)responseObject["main"]["temp_min"] * 10) / 10;
                currentWeather.maxTemp = floor((double_t)responseObject["main"]["temp_max"] * 10) / 10;

                currentWeather.weatherConditionCode = int(responseObject["weather"][0]["id"]);

                const char *weatherDescription = responseObject["weather"][0]["main"]; // JSON.stringify()会把字符串加一对引号
                strncpy(currentWeather.weatherDescription, weatherDescription, sizeof(currentWeather.weatherDescription) - 1);
                // strncpy(dest, src, sizeof(dest) - 1);
                currentWeather.weatherDescription[sizeof(currentWeather.weatherDescription) - 1] = '\0';

                const char *city = responseObject["name"];
                strncpy(currentWeather.city, city, sizeof(currentWeather.city) - 1);
                currentWeather.city[sizeof(currentWeather.city) - 1] = '\0';

                int gmtOffset = int(responseObject["timezone"]);

                // 把时区也加上, 不然时间不对
                breakTime((time_t)(int)responseObject["sys"]["sunrise"] + gmtOffset, currentWeather.sunrise);
                breakTime((time_t)(int)responseObject["sys"]["sunset"] + gmtOffset, currentWeather.sunset);

                lastFetchWeatherMinute = curMinute;

                Serial.println("Get Weather from net ok");
                Serial.println(payload);
            }
            else
            {
                // http error
                Serial.println("Get Weather from net error");
                currentWeather.ok = false;
            }
            http.end();
        }
        else // No WiFi, use internal temperature sensor
        {
            weather_wifiConnected = false;
            Serial.println("No WiFi, use internal temperature sensor");
            currentWeather.ok = false;
        }
    }
    else
    {
        Serial.println("Get weather from cache");
    }
    return true;
}

static void doSyncWeather()
{
    weather_syncing = true;
    weather_syncOK = syncWeather();
    weather_syncing = false;
}

weatherData getWeather () {
    return currentWeather;
}

static display_t draw()
{
    char buf[24];
    uint8_t y = 0;
    if (weather_syncing) {
        draw_string("Syncing ... ", false, 0, y);
        y += 9;
    }
    else {
        if (!currentWeather.ok) {
            if (!weather_wifiConnected) {
                draw_string("Wifi error", false, 0, y);
                return DISPLAY_BUSY;
            }
            draw_string("Syncing error", false, 0, y);
            return DISPLAY_BUSY;
        }
        draw_string_center(currentWeather.city, false, 0, 127, y);
        y += 9;
        draw_fill_screen(0, y, 127, y, 1);
        y += 5;
        
        sprintf(buf, "%s %.1fC", currentWeather.weatherDescription, currentWeather.temperature);
        draw_string(buf, false, 0, y);

        // draw_fill_screen(0, y, 127, y, 1);
        // y++;

        sprintf(buf, "Sunrise: %02d:%02d", currentWeather.sunrise.Hour, currentWeather.sunrise.Minute);
        draw_string(buf, false, 0, y += 9);
        sprintf(buf, "Sunset: %02d:%02d", currentWeather.sunset.Hour, currentWeather.sunset.Minute);
        draw_string(buf, false, 0, y += 9);

        // display.print("Updated:");
        RTC.readDatetime(timeDate);
        uint16_t curMinute = timeDate.date.date * 24 * 60 + timeDate.time.hour * 60 + timeDate.time.mins;
        if (curMinute - lastFetchWeatherMinute <= 1) {
            draw_string("Sync Just now", false, 0, y += 9);
        } else {
            sprintf(buf, "Sync %d Mins ago", curMinute - lastFetchWeatherMinute);
            draw_string(buf, false, 0, y += 9);
        }
    }

    return DISPLAY_BUSY;
}

static bool retrySyncWeather()
{
    doSyncWeather();
    return true;
}

bool showWeather(void)
{
    display_setDrawFunc(draw);
    buttons_setFuncs(retrySyncWeather, btnExit, retrySyncWeather);
    showMeThenRun(NULL);
    keep_on = true;
    doSyncWeather();

    return true;
}