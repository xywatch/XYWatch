#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "common.h"
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <Arduino_JSON.h>

// BLE Setting 服务和特征的 UUID
#define BLE_SETTING_SERVICE_UUID "0000FFD0-0000-1000-8000-00805F9B34FB"
#define BLE_SETTING_CHARACTERISTIC_UUID "0000FFD1-0000-1000-8000-00805F9B34FB"

static BLEServer* pServer = nullptr;
static BLEService* pService = nullptr;
static BLECharacteristic *pCharacteristic = nullptr;

static bool deviceConnected = false;
static bool oldDeviceConnected = false;

static u8 updateStatus = 0; // 1 ok, 2 failed

// 配置项
char setting_name[5] = "life";
// char wifiName[32] = ""; 
// char wifiPassword[32] = "";
u8 setting_age = 25;  // 年龄

// 将当前配置转换为JSON字符串
static String getSettingsJson() {
    JSONVar doc;
    doc["name"] = setting_name;
    doc["wifiName"] = appConfig.wifiName;
    doc["wifiPassword"] = appConfig.wifiPassword;
    doc["age"] = setting_age;
    return JSON.stringify(doc);
}

// 从JSON字符串更新配置
static bool updateSettingsFromJson(const char* jsonString) {
    JSONVar doc = JSON.parse(jsonString);
    
    if (JSON.typeof(doc) == "undefined") {
        Serial.println("JSON parsing failed!");
        updateStatus = 2;
        return false;
    }
    
    // 更新配置
    if (doc.hasOwnProperty("name")) {
        const char* name = (const char*)doc["name"];
        strlcpy(setting_name, name, sizeof(setting_name));
    }
    if (doc.hasOwnProperty("wifiName")) {
        const char* fname = (const char*)doc["wifiName"];
        strlcpy(appConfig.wifiName, fname, sizeof(appConfig.wifiName));
    }
    if (doc.hasOwnProperty("wifiPassword")) {
        const char* lname = (const char*)doc["wifiPassword"];
        strlcpy(appConfig.wifiPassword, lname, sizeof(appConfig.wifiPassword));
    }
    if (doc.hasOwnProperty("age")) {
        setting_age = (u8)doc["age"];
    }
    updateStatus = 1;

    appconfig_save();
    
    return true;
}

// Add BLE update function for settings service
static void ble_setting_update()
{
    // 断开连接后自动重新广播
    if (!deviceConnected && oldDeviceConnected)
    {
        delay(500);
        pServer->startAdvertising();
        oldDeviceConnected = deviceConnected;
        Serial.println("BLE Setting: Restarting advertising");
    }

    if (deviceConnected && !oldDeviceConnected)
    {
        oldDeviceConnected = deviceConnected;
        Serial.println("BLE Setting: Device connected");
        // 连接后立即发送当前配置
        String jsonString = getSettingsJson();
        pCharacteristic->setValue(jsonString.c_str());
        pCharacteristic->notify();
    }
}

class BLESettingServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        deviceConnected = true;
        Serial.println("Device connected");
        updateStatus = 0;
    }

    void onDisconnect(BLEServer *pServer)
    {
        deviceConnected = false;
        Serial.println("Device disconnected");
    }
};

class BLESettingCallback : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        std::string rxValue = pCharacteristic->getValue();
        Serial.printf("Received data: %s\n", rxValue.c_str());
        
        // 更新配置
        if (updateSettingsFromJson(rxValue.c_str())) {
            Serial.println("Settings updated successfully");
            // 发送更新后的配置回客户端确认
            String jsonString = getSettingsJson();
            pCharacteristic->setValue(jsonString.c_str());
            pCharacteristic->notify();
        }
    }
    
    void onRead(BLECharacteristic *pCharacteristic)
    {
        // 读取请求时返回当前配置
        String jsonString = getSettingsJson();
        pCharacteristic->setValue(jsonString.c_str());
    }
};

// 初始化OTA
void initBLESetting()
{
    // 初始化BLE设备
    BLEDevice::init("XYWatch-Settings");

    // 设置发射功率为最大
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P9);

    // 创建BLE服务器
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new BLESettingServerCallbacks());

    // 创建OTA服务
    pService = pServer->createService(BLE_SETTING_SERVICE_UUID);

    // 创建OTA特征
    pCharacteristic = pService->createCharacteristic(
        BLE_SETTING_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setCallbacks(new BLESettingCallback());

    // 启动服务
    pService->start();

    // 开始广播
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BLE_SETTING_SERVICE_UUID);
    pAdvertising->setScanResponse(true);

    // 设置更快的广播间隔
    pAdvertising->setMinInterval(0x20); // 最小间隔
    pAdvertising->setMaxInterval(0x40); // 最大间隔

    BLEDevice::startAdvertising();

    Serial.println("BLE Setting Ready!");
}

void stopBLESetting() {
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

static uint8_t ble_y = 0;

static bool btnExit()
{
    /*
    if (deviceConnected) {
        // 只会闪一下, 因为会一直刷新
        // draw_string("Please disconnect to exit", false, 0, ble_y + 8);
        return true;
    }
    */

    stopBLESetting();

    keep_on = false; // 如果在draw里一直刷新变量，变量在这里关闭不了，因为会draw线程会再次覆盖
    back_to_watchface();
    return true;
}

static display_t draw()
{
    ble_setting_update();
    ble_y = 0;

    if (deviceConnected) {
        draw_string("Device Connected", false, 0, 0);
        if (updateStatus == 1) {
            draw_string("Setup success", false, 0, ble_y += 9);
        }
        else if (updateStatus == 2) {
            draw_string("Setup failed", false, 0, ble_y += 9);
        }
    } else {
        draw_string("Bluetooth Started", false, 0, 0);
        draw_string("XYWatch-Settings ", false, 0, ble_y += 9);
        draw_string("Waiting for", false, 0, ble_y += 9);
        draw_string("connection...", false, 0, ble_y += 9);
        // draw_string_center("12.32%", false, 0, 127, ble_y += 9);
        // ble_y += 9;
        // draw_fill_screen(0, ble_y, 85, ble_y, 2);
    }
    return DISPLAY_BUSY;
}

void showBLESetting()
{
    initBLESetting();
    display_setDrawFunc(draw);
    buttons_setFuncs(btnExit, btnExit, btnExit);
    showMeThenRun(NULL);
    keep_on = true;
}