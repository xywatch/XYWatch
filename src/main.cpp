#include <Arduino.h>
#include <Wire.h>
#include <stdio.h>
#include "common.h"
#include "driver/rtc_io.h"

WatchRTC RTC;
extern BMA423 sensor;

// I2C scan function
// 适用于 I2C 设备探测和通信
void I2Cscan()
{
  Wire.begin();
  Serial.println("I2Cscan...");

  // scan for i2c devices
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for (address = 1; address < 127; address++)
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmission to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    // error = Wire.transfer(address, NULL, 0, NULL, 0);

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");

      nDevices++;
    }
    else if (error == 4)
    {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
}

unsigned long last = 0;

// 刷新一次要95ms
/*
刷新率 = 1000ms / 95ms ≈ 10.5 Hz
所以当前的刷新率大约是10.5帧每秒(FPS)。这个刷新率对于动画来说确实偏低，因为：
人眼能感知到的流畅动画通常需要至少24 FPS
标准显示器通常是60 Hz (60 FPS)
一般游戏至少需要30 FPS才能保证较好的体验
对于OLED显示屏来说：
使用I2C接口理想情况下应该能达到30-40 FPS (约25-33ms每帧)

刷新率 = 1000ms / 15ms ≈ 66.7 Hz
这个刷新率已经非常不错了！达到了专业显示器的标准水平（60Hz）。对比之前：
之前：95ms/帧 ≈ 10.5 Hz
现在：15ms/帧 ≈ 66.7 Hz
*/
void showSpeed()
{
  char name[10];
  sprintf(name, "%d", millis() - last);
  last = millis();
  draw_string(name, NOINVERT, 30, 0);
}

void showSpace()
{
  const byte width = 52;
  const byte height = 48;
  const byte x = (128 - width) / 2;
  const byte y = (64 - height) / 2;
  byte count = 2;

  const u8 *space_images[] = {
      space_image1, space_image2, space_image3, space_image4, space_image5,
      space_image6, space_image7, space_image8, space_image9, space_image10};

  while (count--)
  {
    for (int i = 0; i < 10; i++)
    {
      OLED_ClearScreenBuffer(); // 只在开始时清屏一次
      draw_bitmap(x, y, space_images[i], width, height, NOINVERT, 0);
      // showSpeed();
      OLED_Flush();
      delay(80);
    }
  }
}

void testOLED2()
{
  // OLED_Init();
  draw_string("2020-12-31", NOINVERT, 30, 20);
  OLED_Flush();
}

// 在ESP32中，从深度睡眠(deep sleep)唤醒后实际上就是一次重启，它会重新从setup()开始执行
void setup()
{
  // 检查是否是从深度睡眠唤醒
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  bool is_boot = wakeup_reason != ESP_SLEEP_WAKEUP_EXT0 && wakeup_reason != ESP_SLEEP_WAKEUP_EXT1;

  // 初始化第二个 I2C, 低速
  Wire1.begin(SDA_1, SCL_1);
  delay(10);

  Serial.begin(115200);
  Serial.println("Starting...");

  // 初始化OLED I2C, 高速I2C
  Wire.begin(SDA_OLED, SCL_OLED);
  Wire.setClock(800000);
  delay(10);

  RTC.init();
  // RTC.config("2025:03:12:12:00:00"); // 有时时钟芯片会报错, 需要重新初始化时间
  Serial.println("RTC OK");

  // 根据唤醒原因决定是否显示开机动画
  if (is_boot) // 不是从按钮/RTC唤醒, 开机
  {
    appconfig_init(); // 只在冷启动时需要

    OLED_Init();
    showSpace(); // 只在冷启动时显示开机动画

    buzzer_init();
    buzzer_boot(); // 只在冷启动时需要

    if (!bmaConfig()) // 只在冷启动时需要
    {
      Serial.println("BMA423 init failed!");
    }

    // For some reason, seems to be enabled on first boot
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  }
  else
  {
    OLED_Init();
    buzzer_init();

    Serial.printf("wakeup_reason: %s\n", wakeup_reason == ESP_SLEEP_WAKEUP_EXT0 ? "RTC" : "BUTTON");

    Serial.printf("ACC_INT_1_PIN: %d, ACC_INT_2_PIN: %d, RTC_INT_PIN: %d\n", digitalRead(ACC_INT_1_PIN), digitalRead(ACC_INT_2_PIN), digitalRead(RTC_INT_PIN));
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0)
    {
      RTC.clearAlarm(); // 清除闹钟, 中断, 也会清除一个闹钟
      alarm_need_updateAlarm_in_nextLoop(); // 在下一个alarm_update()后再更新alarm
    }
    else
    {
      uint64_t GPIO_mask = esp_sleep_get_ext1_wakeup_status();
      uint64_t GPIO_number = log(GPIO_mask) / log(2);
      Serial.printf("Wake up caused because of GPIO: %d\n", GPIO_number);

      if (GPIO_number == ACC_INT_1_PIN || GPIO_number == ACC_INT_2_PIN)
      {
        // 检查是否是BMA423唤醒
        uint16_t int_status = sensor.getInterruptStatus();
        if (int_status)
        {
          // uint32_t stepCount = sensor.getCounter();
          Serial.printf("BMA423 int status: %d\n", int_status);
          if (sensor.isDoubleClick())
          {
            Serial.println("is double click");
          }
          else if (sensor.isTilt())
          {
            Serial.println("is wrist tilt");
          }
          else if (sensor.isStepCounter())
          {
            Serial.println("is step count");
            uint32_t stepCount = sensor.getCounter();
            Serial.printf("stepCount: %d\n", stepCount);
          }
        }
      }
      else if (GPIO_number == MENU_BTN_PIN)
      {
        Serial.println("MENU_BTN_PIN");
      }
    }
  }

  buttons_init();
  pwrmgr_init();

  initLED();

  pinMode(USB_DET, INPUT);
  pinMode(CHARGE_STAT, INPUT_PULLUP);

  time_update();
  alarm_init();

  if (is_boot)
  {
    OLED_Clear();
    console_log(100, "Start...");
  }

  // 清屏缓冲区
  OLED_ClearScreenBuffer();

  // 启动表盘
  display_set(watchface_normal);
  display_load();
}

extern bool SleepRequested;
void loop()
{
  // 测试
  // 获取BMA423中断状态
  uint16_t int_status = sensor.getInterruptStatus();
  if (int_status)
  {
    // uint32_t stepCount = sensor.getCounter();
    Serial.printf("BMA423 int status: %d\n", int_status);
    if (sensor.isDoubleClick())
    {
      Serial.println("double click");
    }
    if (sensor.isStepCounter())
    {
      Serial.printf("step count %d\n", sensor.getCounter());
    }
    if (sensor.isTilt())
    {
      Serial.println("wrist tilt");
    }
  }

  // ble_update();
  ble_update();

  if (SleepRequested)
  {
    if(enterSleep()) {
      return;
    }
  }

  // 添加简单的看门狗机制
  static unsigned long lastMillis = 0;
  if (millis() - lastMillis > 5000)
  {
    lastMillis = millis();
    blinkLED();
    // 发送心跳信号
    Serial.println("heartbeat");
  }

  buttons_update();
  pwrmgr_update();
  buzzer_update(); // 在loop中调用

  // 刷新屏幕
  if (pwrmgr_userActive())
  {
    time_update();

#if COMPILE_STOPWATCH
    stopwatch_update();
#endif

    alarm_update();

    // 刷新屏幕
    display_update();
  }

  if (display_is_enabled())
  {
    // 显示完成后清除缓冲区
    OLED_ClearScreenBuffer();
  }
}
