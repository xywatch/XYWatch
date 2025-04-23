#include "common.h"

// LEDC配置
#define LEDC_CHANNEL_0     0
#define LEDC_TIMER_10_BIT  10    // 改用10位分辨率
#define LEDC_BASE_FREQ     2000  // 降低基础频率


// PWM参数
#define PWM_MAX_DUTY       1023  // 10位分辨率下的最大值
#define PWM_HIGH_DUTY      920   // 90%占空比
#define PWM_MID_DUTY       716   // 70%占空比
#define PWM_LOW_DUTY       512   // 50%占空比

static uint buzzLen;
static millis8_t startTime;
static buzzFinish_f onFinish;
static tonePrio_t prio;

static void stop(void);

// 蜂鸣器测试
void buzzer_test()
{
    Serial.println("Buzzer frequency scan test...");
    
    // 测试1：低频范围（200-1000Hz）
    Serial.println("\nTesting low frequencies (200-1000Hz)...");
    for(uint16_t freq = 200; freq <= 1000; freq += 100) {
        Serial.printf("Testing frequency: %dHz\n", freq);
        ledcWriteTone(LEDC_CHANNEL_0, freq);
        ledcWrite(LEDC_CHANNEL_0, PWM_HIGH_DUTY);
        delay(300);
    }
    
    // 测试2：中频范围（1000-2000Hz）
    Serial.println("\nTesting mid frequencies (1000-2000Hz)...");
    for(uint16_t freq = 1000; freq <= 2000; freq += 200) {
        Serial.printf("Testing frequency: %dHz\n", freq);
        ledcWriteTone(LEDC_CHANNEL_0, freq);
        ledcWrite(LEDC_CHANNEL_0, PWM_HIGH_DUTY);
        delay(300);
    }
    
    // 测试3：高频范围（2000-5000Hz）
    Serial.println("\nTesting high frequencies (2000-5000Hz)...");
    for(uint16_t freq = 2000; freq <= 5000; freq += 500) {
        Serial.printf("Testing frequency: %dHz\n", freq);
        ledcWriteTone(LEDC_CHANNEL_0, freq);
        ledcWrite(LEDC_CHANNEL_0, PWM_HIGH_DUTY * 8 / 10);  // 高频降低音量
        delay(300);
    }
    
    // 测试4：音量级别
    Serial.println("\nTesting volume levels at 1kHz...");
    ledcWriteTone(LEDC_CHANNEL_0, 1000);
    
    Serial.println("Testing LOW volume");
    ledcWrite(LEDC_CHANNEL_0, PWM_LOW_DUTY);
    delay(1000);
    
    Serial.println("Testing MID volume");
    ledcWrite(LEDC_CHANNEL_0, PWM_MID_DUTY);
    delay(1000);
    
    Serial.println("Testing HIGH volume");
    ledcWrite(LEDC_CHANNEL_0, PWM_HIGH_DUTY);
    delay(1000);
    
    // 测试5：音乐音阶（C大调）
    Serial.println("\nTesting musical scale (C Major)...");
    uint16_t notes[] = {262, 294, 330, 349, 392, 440, 494, 523};  // C4-C5
    for(int i = 0; i < 8; i++) {
        ledcWriteTone(LEDC_CHANNEL_0, notes[i]);
        ledcWrite(LEDC_CHANNEL_0, PWM_HIGH_DUTY);
        delay(200);
        ledcWrite(LEDC_CHANNEL_0, 0);
        delay(50);
    }
    
    Serial.println("Buzzer test end");
    stop();
}

// 开机提示音 - 使用中音A (440Hz)
void buzzer_boot()
{
    ledcWriteTone(LEDC_CHANNEL_0, 440);
    ledcWrite(LEDC_CHANNEL_0, PWM_HIGH_DUTY);
    delay(100);
    stop();
}

// 蜂鸣器初始化
void buzzer_init()
{
    ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_10_BIT);
    ledcAttachPin(BUZZER_PIN, LEDC_CHANNEL_0);
}

void buzzer_buzz(uint16_t len, uint16_t tone, vol_t volType, tonePrio_t _prio, buzzFinish_f _onFinish)
{
    if (_prio < prio)
        return;
    
    if (tone == TONE_STOP)
    {
        stop();
        return;
    }

    prio = _prio;
    onFinish = _onFinish;
    buzzLen = len;
    startTime = millis();

    if (tone == TONE_PAUSE)
    {
        ledcWrite(LEDC_CHANNEL_0, 0);
        return;
    }

    uint16_t volume;
    byte vol;

    switch (volType)
    {
        case VOL_UI:
            vol = appConfig.volUI;
            break;
        case VOL_ALARM:
            vol = appConfig.volAlarm;
            break;
        case VOL_HOUR:
            vol = appConfig.volHour;
            break;
        default:
            vol = 2;
            break;
    }

    // 音量控制 - 使用不同占空比
    switch (vol)
    {
        case 0:
            volume = PWM_MAX_DUTY;  // 关闭
            return;
            break;
        case 1:
            volume = PWM_LOW_DUTY;  // 50%占空比
            break;
        case 2:
            volume = PWM_MID_DUTY;  // 70%占空比
            break;
        case 3:
            volume = PWM_HIGH_DUTY; // 90%占空比
            break;
        default:
            volume = PWM_MID_DUTY;
            break;
    }

    // 频率补偿
    uint32_t freq = tone;
    uint32_t duty = volume;

    /*
    // 低频补偿（<500Hz）
    if (freq < 500) {
        duty = PWM_HIGH_DUTY;  // 低音使用最大音量
    }
    // 中频优化（500-2000Hz）
    else if (freq < 2000) {
        // 保持原有音量
    }
    // 高频补偿（>2000Hz）
    else {
        // 高频适当降低音量以改善音色
        duty = (duty * 8) / 10;  // 降低到原来的80%
    }
    */

    ledcWriteTone(LEDC_CHANNEL_0, freq);
    ledcWrite(LEDC_CHANNEL_0, duty);
    
    Serial.printf("Buzzer: freq=%d, duty=%d, len=%d\n", freq, duty, buzzLen);
}

// 检查是否正在播放
bool buzzer_buzzing()
{
    return buzzLen;
}

// 更新蜂鸣器状态
void buzzer_update()
{
    if (buzzLen && (millis() - startTime) >= buzzLen)
    {
        stop();
        if (onFinish != NULL)
        {
            onFinish(); // 播放下一个音符
        }
    }
}

// 停止播放
void buzzer_stop()
{
    stop();
}

static void stop()
{
    ledcWrite(LEDC_CHANNEL_0, 0);
    buzzLen = 0;
    prio = PRIO_MIN;
}