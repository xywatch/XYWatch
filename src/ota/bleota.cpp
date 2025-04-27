#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "esp_ota_ops.h"
#include "common.h"
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>

// OTA 服务和特征的 UUID
#define OTA_SERVICE_UUID "0000FFC0-0000-1000-8000-00805F9B34FB"
#define OTA_CHARACTERISTIC_UUID "0000FFC1-0000-1000-8000-00805F9B34FB"

BLEServer* pServer = nullptr;
BLEService* pService = nullptr;
BLECharacteristic *pCharacteristic = nullptr;

bool deviceConnected = false;
bool oldDeviceConnected = false;

// OTA 更新相关变量
int content_length = 0;
int content_index = 0;
bool isOTARunning = false;
bool otaSuccess = false;

// loop中调用
void ble_update()
{
    // 断开连接后自动重新广播
    if (!deviceConnected && oldDeviceConnected)
    {
        delay(500);
        pServer->startAdvertising();
        oldDeviceConnected = deviceConnected;
        Serial.println("BLE: Restarting advertising");
        // 打印当前状态
        // Serial.printf("BLE: TX Power: %d\n", esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_DEFAULT));
    }

    if (deviceConnected && !oldDeviceConnected)
    {
        oldDeviceConnected = deviceConnected;
        Serial.println("BLE: Device connected");
    }
}

class ServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        deviceConnected = true;
        Serial.println("Device connected");
        content_index = 0;
        content_length = 0;
        isOTARunning = false;
    }

    void onDisconnect(BLEServer *pServer)
    {
        deviceConnected = false;
        isOTARunning = false;
        Serial.println("Device disconnected");
    }
};

class OTACallback : public BLECharacteristicCallbacks
{
private:
    esp_ota_handle_t update_handle = 0;
    const esp_partition_t *update_partition = NULL;

    /*
    Running partition: app0
    OTA Partition size: 1966080 bytes
    Firmware size: 1775952 bytes
    */
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        std::string rxValue = pCharacteristic->getValue();

        if (!isOTARunning)
        { // 开始OTA
            content_length = *((int *)rxValue.c_str());
            Serial.printf("OTA Start, size: %d\n", content_length);

            const esp_partition_t *running = esp_ota_get_running_partition();
            Serial.printf("Running partition: %s\n", running->label);

            // 获取更新分区
            update_partition = esp_ota_get_next_update_partition(NULL);
            if (update_partition == NULL)
            {
                Serial.println("No OTA partition found");
                return;
            }

            // 3. 确保不是同一个分区
            if (update_partition == running)
            {
                Serial.println("Error: Update partition is same as running partition");
                return;
            }

            // 开始OTA，获取handle
            // OTA_WITH_SEQUENTIAL_WRITES
            // OTA_SIZE_UNKNOWN
            esp_err_t err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
            if (err != ESP_OK)
            {
                Serial.printf("esp_ota_begin failed %d\n", err);

                // 打印具体错误
                switch (err)
                {
                case ESP_ERR_NO_MEM:
                    Serial.println("Not enough memory");
                    break;
                case ESP_ERR_INVALID_ARG:
                    Serial.println("Invalid argument");
                    break;
                case ESP_ERR_OTA_PARTITION_CONFLICT:
                    Serial.println("Partition conflict");
                    break;
                default:
                    Serial.println("Unknown error");
                }
                return;
            }

            Serial.printf("OTA Partition: %s\n", update_partition->label);
            Serial.printf("OTA Partition size: %d bytes\n", update_partition->size);
            Serial.printf("Firmware size: %d bytes\n", content_length);

            isOTARunning = true;
            content_index = 0;

            // 处理第一块数据
            if (rxValue.length() > 4)
            {
                err = esp_ota_write(update_handle, rxValue.c_str() + 4, rxValue.length() - 4);
                if (err != ESP_OK)
                {
                    Serial.println("esp_ota_write failed!");
                    esp_ota_abort(update_handle);
                    isOTARunning = false;
                    return;
                }
                content_index += rxValue.length() - 4;
            }
        }
        else
        { // OTA进行中
            if (content_index < content_length)
            {
                // 使用已存储的 update_handle 继续写入
                esp_err_t err = esp_ota_write(update_handle, rxValue.c_str(), rxValue.length());
                if (err != ESP_OK)
                {
                    Serial.println("esp_ota_write failed!");
                    esp_ota_abort(update_handle);
                    isOTARunning = false;
                    return;
                }
                content_index += rxValue.length();

                Serial.printf("OTA Progress: %0.2f%%\n", ((float)content_index * 100) / content_length);

                // 完成
                if (content_index >= content_length)
                {
                    err = esp_ota_end(update_handle);
                    if (err != ESP_OK)
                    {
                        Serial.println("esp_ota_end failed!");
                        isOTARunning = false;
                        return;
                    }

                    err = esp_ota_set_boot_partition(update_partition);
                    if (err != ESP_OK)
                    {
                        Serial.println("esp_ota_set_boot_partition failed!");
                        isOTARunning = false;
                        return;
                    }

                    Serial.println("OTA Success! Rebooting...");
                    otaSuccess = true;
                    delay(1000);   // 等待串口消息发送完成
                    esp_restart(); // 重启
                }
            }
        }
    }
};

// 初始化OTA
void initBLEOTA()
{
    // 初始化BLE设备
    BLEDevice::init("XYWatch-OTA");

    // 设置发射功率为最大
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P9);

    // 创建BLE服务器
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    // 创建OTA服务
    pService = pServer->createService(OTA_SERVICE_UUID);

    // 创建OTA特征
    pCharacteristic = pService->createCharacteristic(
        OTA_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY);
    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setCallbacks(new OTACallback());

    // 启动服务
    pService->start();

    // 开始广播
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(OTA_SERVICE_UUID);
    pAdvertising->setScanResponse(true);

    // 设置更快的广播间隔
    pAdvertising->setMinInterval(0x20); // 最小间隔
    pAdvertising->setMaxInterval(0x40); // 最大间隔

    BLEDevice::startAdvertising();

    Serial.println("BLE OTA Ready!");
}

void stopBLEOTA() {
    // 1. 停止广播
     if (BLEDevice::getAdvertising()) {
        BLEDevice::getAdvertising()->stop();
    }
    // 停止服务前先断开连接
    if (pServer) {
        pServer->removeService(pService);
    }

    // 4. 关闭 BLE 不行, 不然重新打开会崩溃
    // BLEDevice::deinit(true);
}

extern bool keep_on;

uint8_t ble_y = 0;

static bool btnExit()
{
    /*
    if (isOTARunning) {
        return true;
    }
    if (deviceConnected) {
        // 只会闪一下, 因为会一直刷新
        // draw_string("Please disconnect to exit", false, 0, ble_y + 8);
        return true;
    }
    */

    stopBLEOTA();
    isOTARunning = false;

    keep_on = false; // 如果在draw里一直刷新变量，变量在这里关闭不了，因为会draw线程会再次覆盖
    back_to_watchface();
    return true;
}

static display_t draw()
{
    ble_update();
    ble_y = 0;

    if (deviceConnected) {
        draw_string("Device Connected", false, 0, 0);
        if (!isOTARunning) {
            draw_string("Please upload", false, 0, ble_y += 9);
            draw_string("firmware.bin", false, 0, ble_y += 9);
            return DISPLAY_BUSY;
        }
        draw_string("OTA Progress:", false, 0, ble_y += 9);
        char buf[10];
        sprintf(buf, "%0.2f%%", ((float)content_index * 100) / content_length);
        // draw_string(buf, false, 0, ble_y += 9);
        draw_string_center(buf, false, 0, 127, ble_y += 10);
        ble_y += 9;
        byte progressX = ((float)content_index * 127) / content_length;
        draw_fill_screen(0, ble_y, progressX, ble_y, 2);
        if (otaSuccess) {
            draw_string("OTA Success!", false, 0, ble_y += 9);
            draw_string("Rebooting...", false, 0, ble_y += 9);
        }
    } else {
        draw_string("Bluetooth Started", false, 0, 0);
        draw_string("XYWatch-OTA ", false, 0, ble_y += 9);
        draw_string("Waiting for", false, 0, ble_y += 9);
        draw_string("connection...", false, 0, ble_y += 9);
        // draw_string_center("12.32%", false, 0, 127, ble_y += 9);
        // ble_y += 9;
        // draw_fill_screen(0, ble_y, 85, ble_y, 2);
    }
    return DISPLAY_BUSY;
}

void showUpdateFirmware()
{
    initBLEOTA();
    display_setDrawFunc(draw);
    buttons_setFuncs(btnExit, btnExit, btnExit);
    showMeThenRun(NULL);
    keep_on = true;
}