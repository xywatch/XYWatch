#include "common.h"
#include "nvs_flash.h"

RTC_DATA_ATTR appconfig_s appConfig; // appconfig_s的长度为8
extern alarm_s eepAlarms[];
extern stepLogData_s stepLogs[];

// 写到nvs中
void appconfig_save()
{
    nvs_handle_t handle;
    // 打开命名空间
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err != ESP_OK) return;

    // 写入
    nvs_set_str(handle, "wifiName", appConfig.wifiName);
    nvs_set_str(handle, "wifiPassword", appConfig.wifiPassword);

    nvs_set_u8(handle, "sleepTimeout", appConfig.sleepTimeout);
    nvs_set_u8(handle, "invert", appConfig.invert);
    nvs_set_u8(handle, "animations", appConfig.animations);
    nvs_set_u8(handle, "display180", appConfig.display180);
    nvs_set_u8(handle, "showFPS", appConfig.showFPS);
    nvs_set_u8(handle, "tiltWrist", appConfig.tiltWrist);
    nvs_set_u8(handle, "doubleTap", appConfig.doubleTap);
    
    nvs_set_u8(handle, "timeMode", appConfig.timeMode);
    nvs_set_u8(handle, "volUI", appConfig.volUI);
    nvs_set_u8(handle, "volAlarm", appConfig.volAlarm);
    nvs_set_u8(handle, "volHour", appConfig.volHour);
    nvs_set_u8(handle, "brightness", appConfig.brightness);

    // 提交更改
    nvs_commit(handle);
    
    // 关闭句柄
    nvs_close(handle);

    Serial.printf("nvs save ok\n");
}

void appconfig_init()
{
    // 初始化 NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS 分区被损坏或版本不匹配时擦除重新初始化
        nvs_flash_erase();
        err = nvs_flash_init();
        Serial.printf("nvs_flash_init error: %d\n", err);
    }

    nvs_handle_t handle;
    err = nvs_open("storage", NVS_READONLY, &handle);
    if (err != ESP_OK) {
        Serial.printf("nvs_open error: %d, %s\n", err, esp_err_to_name(err)); // 4354, ESP_ERR_NVS_NOT_FOUND, 第一次没有这个命名空间, 所以要先写一次
        // 第一次运行时，创建命名空间并写入默认值
        // strlcpy(appConfig.wifiName, "", sizeof(appConfig.wifiName));
        // strlcpy(appConfig.wifiPassword, "", sizeof(appConfig.wifiPassword));

        appConfig.sleepTimeout = 2; // 4: 20s; 3: 15s; 2: 10s
        appConfig.invert = false;
        appConfig.animations = true;
        appConfig.display180 = false;
        appConfig.CTRL_LEDs = false;
        appConfig.showFPS = false;
        appConfig.tiltWrist = true;
        appConfig.doubleTap = true;
        appConfig.timeMode = TIMEMODE_24HR;
        // appConfig.volumes = 255;

        appConfig.volUI = 2;
        appConfig.volAlarm = 3;
        appConfig.volHour = 1;

        appConfig.brightness = 2;

        appconfig_save();
        return;
    }

    size_t len = sizeof(appConfig.wifiName);
    err = nvs_get_str(handle, "wifiName", appConfig.wifiName, &len);
    if (err == ESP_OK) {
        Serial.printf("nvs wifiName: %s\n", appConfig.wifiName);
    }

    err = nvs_get_str(handle, "wifiPassword", appConfig.wifiPassword, &len);
    if (err == ESP_OK) {
        Serial.printf("nvs wifiPassword: %s\n", appConfig.wifiPassword);
    }

    // 读取整数
    u8 sleepTimeout = 0;
    err = nvs_get_u8(handle, "sleepTimeout", &sleepTimeout);
    if (err == ESP_OK) {
        Serial.printf("nvs sleepTimeout: %d\n", sleepTimeout);
    }

    u8 invert = false;
    err = nvs_get_u8(handle, "invert", &invert);
    if (err == ESP_OK) {
        Serial.printf("nvs invert: %d\n", invert);
    }

    u8 animations = false;
    err = nvs_get_u8(handle, "animations", &animations);
    if (err == ESP_OK) {
        Serial.printf("nvs animations: %d\n", animations);
    }

    u8 display180 = false;
    err = nvs_get_u8(handle, "display180", &display180);
    if (err == ESP_OK) {
        Serial.printf("nvs display180: %d\n", display180);
    }

    u8 showFPS = false;
    err = nvs_get_u8(handle, "showFPS", &showFPS);
    if (err == ESP_OK) {
        Serial.printf("nvs showFPS: %d\n", showFPS);
    }

    u8 tiltWrist = true;
    err = nvs_get_u8(handle, "tiltWrist", &tiltWrist);
    if (err == ESP_OK) {
        Serial.printf("nvs tiltWrist: %d\n", tiltWrist);
    }

    u8 doubleTap = true;
    err = nvs_get_u8(handle, "doubleTap", &doubleTap);
    if (err == ESP_OK) {
        Serial.printf("nvs doubleTap: %d\n", doubleTap);
    }

    u8 timeMode = 0;
    err = nvs_get_u8(handle, "timeMode", &timeMode);
    if (err == ESP_OK) {
        Serial.printf("nvs timeMode: %d\n", timeMode);
    }

    u8 volUI = 2;
    err = nvs_get_u8(handle, "volUI", &volUI);
    if (err == ESP_OK) {
        Serial.printf("nvs volUI: %d\n", volUI);
    }

    u8 volAlarm = 3;
    err = nvs_get_u8(handle, "volAlarm", &volAlarm);
    if (err == ESP_OK) {
        Serial.printf("nvs volAlarm: %d\n", volAlarm);
    }

    u8 volHour = 1;
    err = nvs_get_u8(handle, "volHour", &volHour);
    if (err == ESP_OK) {
        Serial.printf("nvs volHour: %d\n", volHour);
    }

    u8 brightness = 2;
    err = nvs_get_u8(handle, "brightness", &brightness);
    if (err == ESP_OK) {
        Serial.printf("nvs brightness: %d\n", brightness);
    }

    nvs_close(handle);

    appConfig.sleepTimeout = sleepTimeout == 0 ? 2 : sleepTimeout; // 4: 20s; 3: 15s; 2: 10s
    appConfig.invert = invert;
    appConfig.animations = animations;
    appConfig.display180 = display180;
    appConfig.showFPS = showFPS;
    appConfig.tiltWrist = tiltWrist;
    appConfig.doubleTap = doubleTap;
    appConfig.timeMode = (timemode_t)timeMode; // TIMEMODE_24HR;

    appConfig.volUI = volUI;
    appConfig.volAlarm = volAlarm;
    appConfig.volHour = volHour;

    appConfig.brightness = brightness;

    appConfig.CTRL_LEDs = false; // 没用
}

void appconfig_save_alarm()
{
    nvs_handle_t handle;
    // 打开命名空间
    esp_err_t err = nvs_open("alarm", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        Serial.printf("nvs_open NVS_READWRITE alarm error: %d, %s\n", err, esp_err_to_name(err)); // 4354, ESP_ERR_NVS_NOT_FOUND, 第一次没有这个命名空间, 所以要先写一次
        return;
    }

    nvs_set_blob(handle, "alarms", eepAlarms, sizeof(alarm_s) * ALARM_COUNT);

    // 提交更改
    nvs_commit(handle);
    
    // 关闭句柄
    nvs_close(handle);

    Serial.printf("nvs save alarm ok\n");
}

void appconfig_init_alarm()
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("alarm", NVS_READONLY, &handle);
    if (err != ESP_OK) {
        Serial.printf("nvs_open alarm error: %d, %s\n", err, esp_err_to_name(err)); // 4354, ESP_ERR_NVS_NOT_FOUND, 第一次没有这个命名空间, 所以要先写一次
        appconfig_save_alarm();
        return;
    }

    size_t len = sizeof(alarm_s) * ALARM_COUNT;
    err = nvs_get_blob(handle, "alarms", &eepAlarms, &len);
    if (err == ESP_OK) {
        Serial.printf("eepAlarm 0 %d, %d:%d\n", eepAlarms[0].enabled, eepAlarms[0].hour, eepAlarms[0].min);
    }

    nvs_close(handle);
}

void appconfig_save_step_log()
{
    nvs_handle_t handle;
    // 打开命名空间
    esp_err_t err = nvs_open("stepLogs", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        Serial.printf("nvs_open NVS_READWRITE stepLog error: %d, %s\n", err, esp_err_to_name(err)); // 4354, ESP_ERR_NVS_NOT_FOUND, 第一次没有这个命名空间, 所以要先写一次
        return;
    }

    nvs_set_blob(handle, "stepLogs", stepLogs, sizeof(stepLogData_s) * STEP_LOG_COUNT);

    // 提交更改
    nvs_commit(handle);
    
    // 关闭句柄
    nvs_close(handle);

    Serial.printf("nvs save stepLogs ok\n");
}

void appconfig_init_step_log()
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("stepLogs", NVS_READONLY, &handle);
    if (err != ESP_OK) {
        Serial.printf("nvs_open stepLogs error: %d, %s\n", err, esp_err_to_name(err)); // 4354, ESP_ERR_NVS_NOT_FOUND, 第一次没有这个命名空间, 所以要先写一次
        appconfig_save_step_log();
        return;
    }

    size_t len = sizeof(stepLogData_s) * STEP_LOG_COUNT;
    err = nvs_get_blob(handle, "stepLogs", &stepLogs, &len);
    if (err == ESP_OK) {
        Serial.printf("stepLogs 0 %d, %d-%d\n", stepLogs[0].stepCount, stepLogs[0].month, stepLogs[0].date);
    }

    nvs_close(handle);
}