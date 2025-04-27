#include "ota/netota.h"
#include "esp_ota_ops.h"
#include <HTTPClient.h>
#include <Update.h>

extern bool keep_on;

// 网络OTA更新相关变量
bool netOTAConnected = false;
bool netOTARunning = false;
bool netOTASuccess = false;
int netOTAContentLength = 0;
int netOTAContentIndex = 0;
bool hasNewVesrion = false;

// 检查是否有新版本可用
bool checkForUpdate()
{
    hasNewVesrion = false;
    if (!connectWiFi())
    {
        Serial.println("WiFi connection failed");
        console_log(50, "WiFi connection failed");
        return false;
    }

    HTTPClient http;
    http.begin(OTA_CHECK_URL);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK)
    {
        String payload = http.getString();
        // 解析JSON响应，检查版本
        // 这里简化处理，实际应用中应该解析JSON比较版本号
        if (payload.indexOf(FW_VERSION) == -1)
        {
            // 有新版本
            http.end();
            hasNewVesrion = true;
            console_log(50, "New version: %s", payload);
            console_log(50, "Please press Confirm button to update");
            return true;
        }
    }

    http.end();
    console_log(50, "No new version");
    return false;
}

// 执行OTA更新
bool performOTAUpdate()
{
    Serial.println("WiFi connect...");
    if (!connectWiFi())
    {
        Serial.println("WiFi connection failed");
        console_log(50, "WiFi connection failed");
        return false;
    }

    HTTPClient http;
    http.begin(OTA_SERVER_URL);
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK)
    {
        Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
        http.end();
        console_log(50, "HTTP GET failed");
        return false;
    }

    // 获取文件大小
    netOTAContentLength = http.getSize();
    Serial.printf("Firmware size: %d bytes\n", netOTAContentLength);
    console_log(50, "Firmware size: %d bytes\n", netOTAContentLength);

    // 检查分区
    const esp_partition_t *running = esp_ota_get_running_partition();
    Serial.printf("Running partition: %s\n", running->label);

    // 获取更新分区
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL)
    {
        Serial.println("No OTA partition found");
        console_log(50, "No OTA partition found");
        http.end();
        return false;
    }

    // 确保不是同一个分区
    if (update_partition == running)
    {
        Serial.println("Error: Update partition is same as running partition");
        console_log(50, "Error: Update partition is same as running partition");
        http.end();
        return false;
    }

    // 开始OTA，获取handle
    esp_ota_handle_t update_handle = 0;
    esp_err_t err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
    if (err != ESP_OK)
    {
        Serial.printf("esp_ota_begin failed %d\n", err);
        console_log(50, "esp_ota_begin failed %d\n", err);
        http.end();
        return false;
    }

    Serial.printf("OTA Partition: %s\n", update_partition->label);
    Serial.printf("OTA Partition size: %d bytes\n", update_partition->size);

    // 创建缓冲区
    uint8_t buff[1024] = {0};
    WiFiClient *stream = http.getStreamPtr();

    // 开始下载并写入
    netOTAContentIndex = 0;
    netOTARunning = true;

    while (netOTAContentIndex < netOTAContentLength && http.connected())
    {
        // 读取数据
        size_t size = stream->available();
        if (size)
        {
            size_t readBytes = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

            // 写入OTA分区
            err = esp_ota_write(update_handle, buff, readBytes);
            if (err != ESP_OK)
            {
                Serial.println("esp_ota_write failed!");
                console_log(50, "esp_ota_begin failed %d\n", err);
                esp_ota_abort(update_handle);
                http.end();
                netOTARunning = false;
                return false;
            }

            netOTAContentIndex += readBytes;
            Serial.printf("Progress: %0.2f%%\n", ((float)netOTAContentIndex * 100) / netOTAContentLength);
            console_log_no_scroll(50, "Progress: %0.2f%%", ((float)netOTAContentIndex * 100) / netOTAContentLength);
        }
        delay(1); // 给系统一些时间处理
    }

    // 完成OTA
    err = esp_ota_end(update_handle);
    if (err != ESP_OK)
    {
        Serial.println("esp_ota_end failed!");
        console_log(50, "esp_ota_begin failed %d\n", err);
        http.end();
        netOTARunning = false;
        return false;
    }

    // 设置启动分区
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK)
    {
        Serial.println("esp_ota_set_boot_partition failed!");
        console_log(50, "esp_ota_set_boot_partition failed!");
        http.end();
        netOTARunning = false;
        return false;
    }

    http.end();
    netOTARunning = false;
    netOTASuccess = true;

    Serial.println("OTA Success! Rebooting...");
    console_log(50, "OTA Success!");
    console_log(50, "Rebooting...");
    delay(1000);   // 等待串口消息发送完成
    esp_restart(); // 重启

    return true;
}

// 初始化网络OTA
void initNetOTA()
{
    netOTAConnected = false;
    netOTARunning = false;
    netOTASuccess = false;
    netOTAContentLength = 0;
    netOTAContentIndex = 0;

    animation_reset_offsetY(); // 重置偏移量, 不然offsetY会导致超出屏幕
    OLED_ClearScreenBuffer();
    console_init(); // 初始化console, 重置从第一行开始

    if (checkForUpdate())
    {
        // performOTAUpdate();
    }
}

// 显示网络OTA更新界面
static bool btnExit()
{
    display_update_enable();
    keep_on = false;
    back_to_watchface();
    return true;
}

static bool confirmUpdate () {
    if (hasNewVesrion) {
        hasNewVesrion = false; // 避免重复操作
        performOTAUpdate();
    } else {
        btnExit();
    }
    return true;
}

// 没用了, 因为wifi获取数据会阻塞进程不会执行loop()
static display_t draw()
{
    uint8_t y = 0;
    if (netOTARunning)
    {
        draw_string("Progress:", false, 0, y);
        char buf[10];
        sprintf(buf, "%0.2f%%", ((float)netOTAContentIndex * 100) / netOTAContentLength);
        draw_string_center(buf, false, 0, 127, y += 10);
        y += 9;
        byte progressX = ((float)netOTAContentIndex * 127) / netOTAContentLength;
        draw_fill_screen(0, y, progressX, y, 2);

        if (netOTASuccess)
        {
            draw_string("OTA Success!", false, 0, y += 9);
            draw_string("Rebooting...", false, 0, y += 9);
        }
    }
    else
    {
        draw_string("Network OTA", false, 0, y);
        draw_string("Checking for", false, 0, y += 9);
        draw_string("updates...", false, 0, y += 9);

        if (netOTASuccess)
        {
            draw_string("Update found!", false, 0, y += 9);
            draw_string("Starting...", false, 0, y += 9);
        }
    }

    return DISPLAY_BUSY;
}

void showNetUpdateFirmware()
{
    // display_setDrawFunc(draw);
    display_setDrawFunc(NULL);
    buttons_setFuncs(btnExit, confirmUpdate, btnExit);
    showMeThenRun(NULL);
    keep_on = true;

    display_update_disable();
    initNetOTA();
}