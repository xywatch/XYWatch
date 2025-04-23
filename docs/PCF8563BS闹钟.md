# PCF8563BS

```
typedef struct  { 
  uint8_t Second; 
  uint8_t Minute; 
  uint8_t Hour; 
  uint8_t Wday;   // 星期几 (1 = 星期日, 7 = 星期六)
  uint8_t Day; // 一个月中的第几天 (1 - 31)
  uint8_t Month; // 月份 1-12
  uint8_t Year;   // offset from 1970; 
} 	tmElements_t, TimeElements, *tmElementsPtr_t;
```

PCF8563 的星期表示中，0表示周日, 6表示周六
day_t 0-6, 0:周一, 6:周日, 3=周四
tmWday 1-7, 1:星期日, 2周一, 3周二, 4周三, 7:星期六, 

PCF8563 的年份寄存器（years 寄存器）是一个 8 位寄存器，存储的值范围是 0x00 到 0x99，分别表示 2000 年到 2099 年。

例如：

寄存器值为 0x00 表示 2000 年。
寄存器值为 0x23 表示 2023 年。
寄存器值为 0x63 表示 2063 年。

注意事项
基准年：
PCF8563 的年份寄存器是以 2000 年为基准，而不是 1900 年。因此，直接读取寄存器的值并加上 2000 即可得到当前年份。
例如，寄存器值为 0x17，表示 2017 年（0x17 是十进制的 23，2000 + 23 = 2023）。
寄存器范围：
PCF8563 的年份寄存器只能表示 2000 年到 2099 年。如果需要处理超出这个范围的年份，需要额外处理。

BCD 格式：
PCF8563 的年份寄存器值是以 BCD（Binary-Coded Decimal） 格式存储的。因此，在读取或写入时需要进行 BCD 和十进制之间的转换。

## 闹钟特性
1. 基本功能：
   - 分钟闹钟
   - 小时闹钟
   - 日期闹钟
   - 工作日闹钟
   - 支持闹钟中断输出

2. 闹钟寄存器：
   - 分钟闹钟寄存器 (0x09)
   - 小时闹钟寄存器 (0x0A)
   - 日期闹钟寄存器 (0x0B)
   - 工作日闹钟寄存器 (0x0C)

## 代码实现

```cpp
// 基本设置闹钟
void setAlarm() {
    Wire.beginTransmission(0x51);  // PCF8563地址
    Wire.write(0x09);  // 闹钟寄存器起始地址
    Wire.write(30);    // 分钟 (30)
    Wire.write(0x08);  // 小时 (8点)
    Wire.write(0x01);  // 日期 (1号)
    Wire.write(0x02);  // 星期 (周一)
    Wire.endTransmission();
    
    // 启用闹钟中断
    Wire.beginTransmission(0x51);
    Wire.write(0x01);  // 控制/状态寄存器2
    Wire.write(0x02);  // 启用闹钟中断
    Wire.endTransmission();
}

// 闹钟中断处理
void handleAlarm() {
    pinMode(ALARM_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ALARM_PIN), alarmCallback, FALLING);
}

void alarmCallback() {
    // 闹钟响应处理
    // 例如：亮屏、震动等
}

// 检查闹钟状态
bool checkAlarm() {
    Wire.beginTransmission(0x51);
    Wire.write(0x01);
    Wire.endTransmission();
    
    Wire.requestFrom(0x51, 1);
    if(Wire.available()) {
        uint8_t status = Wire.read();
        return (status & 0x08); // 检查闹钟标志位
    }
    return false;
}

// 清除闹钟标志
void clearAlarm() {
    Wire.beginTransmission(0x51);
    Wire.write(0x01);
    Wire.write(0x00);  // 清除闹钟标志
    Wire.endTransmission();
}
```

## 闹钟设置选项

1. 每日闹钟：
```cpp
void setDailyAlarm(uint8_t hour, uint8_t minute) {
    Wire.beginTransmission(0x51);
    Wire.write(0x09);
    Wire.write(minute);
    Wire.write(hour);
    Wire.write(0x80);  // 禁用日期匹配
    Wire.write(0x80);  // 禁用星期匹配
    Wire.endTransmission();
}
```

2. 工作日闹钟：
```cpp
void setWeekdayAlarm(uint8_t hour, uint8_t minute) {
    Wire.beginTransmission(0x51);
    Wire.write(0x09);
    Wire.write(minute);
    Wire.write(hour);
    Wire.write(0x80);  // 禁用日期匹配
    Wire.write(0x1F);  // 周一到周五
    Wire.endTransmission();
}
```

3. 单次闹钟：
```cpp
void setOneTimeAlarm(uint8_t hour, uint8_t minute, uint8_t day) {
    Wire.beginTransmission(0x51);
    Wire.write(0x09);
    Wire.write(minute);
    Wire.write(hour);
    Wire.write(day);
    Wire.write(0x80);  // 禁用星期匹配
    Wire.endTransmission();
}
```

## 注意事项

1. 中断处理：
   - 闹钟触发会产生中断信号
   - 需要及时清除中断标志
   - 建议使用中断方式处理

2. 省电考虑：
   - 闹钟功能在深度睡眠时仍然工作
   - 可以用作定时唤醒源
   - 功耗极低

3. 使用建议：
   - 设置闹钟前先清除旧的闹钟标志
   - 定期检查时钟是否正常运行
   - 考虑时区和夏令时影响

## 实际应用示例

```cpp
class AlarmManager {
private:
    static const uint8_t PCF8563_ADDR = 0x51;
    uint8_t alarmPin;

public:
    AlarmManager(uint8_t pin) : alarmPin(pin) {
        pinMode(alarmPin, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(alarmPin), 
                       std::bind(&AlarmManager::handleInterrupt, this), 
                       FALLING);
    }

    void setAlarm(uint8_t hour, uint8_t minute) {
        clearAlarm();  // 清除旧的闹钟标志
        
        Wire.beginTransmission(PCF8563_ADDR);
        Wire.write(0x09);
        Wire.write(minute);
        Wire.write(hour);
        Wire.write(0x80);  // 禁用日期匹配
        Wire.write(0x80);  // 禁用星期匹配
        Wire.endTransmission();
        
        enableAlarmInterrupt();
    }

    void enableAlarmInterrupt() {
        Wire.beginTransmission(PCF8563_ADDR);
        Wire.write(0x01);
        Wire.write(0x02);  // 启用闹钟中断
        Wire.endTransmission();
    }

    void clearAlarm() {
        Wire.beginTransmission(PCF8563_ADDR);
        Wire.write(0x01);
        Wire.write(0x00);
        Wire.endTransmission();
    }

private:
    void handleInterrupt() {
        // 处理闹钟事件
        // 例如：唤醒设备、显示提醒等
    }
};
```
```

这个文档已经创建好了。您需要我对某个部分进行补充或修改吗？ 