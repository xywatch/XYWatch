#ifndef CONFIG_H_
#define CONFIG_H_

// Hardware version
// 1 = PCB 1.0 - 1.1
// 2 = PCB 1.2
// 3 = PCB 1.3 - 1.4
#define HW_VERSION "1"

#define USER_NAME "Life"
// Firmware version
#define FW_VERSION "1.0.1"

// Language
// 0 = English
// 1 = German (not done)
// 2 = Russian
#define LANGUAGE 0

// 编译选项
#define COMPILE_GAME1 1      // 游戏Breakout
#define COMPILE_GAME2 1      // 游戏Car dodge
// #define COMPILE_GAME3 1      // 游戏Flappy thing (not finished) 有bug  未完成，实在做不来
#define COMPILE_GAME_SNAKE 1 // game snake
#define COMPILE_GAME_LIFE 1  // game life

#define COMPILE_ANIMATIONS 1 // 动画
#define COMPILE_STOPWATCH 1  // 秒表
#define COMPILE_TORCH 1      // 手电筒
#define COMPILE_TUNEMAKER 1  // 3D滚动
#define COMPILE_CALENDAR 1   // 日历

#define UP_BTN_PIN GPIO_NUM_9 // 上 sw1 SD2 28 I/O GPIO9,
#define MENU_BTN_PIN GPIO_NUM_4 // confirm sw2
#define DOWN_BTN_PIN GPIO_NUM_10  // 下 sw3 SD3 29 I/O GPIO10,
#define BACK_BTN_PIN -1 // back 没用

#define UP_BTN_MASK  GPIO_SEL_9
#define MENU_BTN_MASK GPIO_SEL_4
#define DOWN_BTN_MASK GPIO_SEL_10

#define BACK_BTN_MASK GPIO_SEL_1 // 没用

#define RTC_TYPE 2 // PCF8563
#define RTC_INT_PIN GPIO_NUM_27 // IO27

#define ACC_INT_1_PIN GPIO_NUM_14 // BMA423 INT1, IO14
#define ACC_INT_2_PIN GPIO_NUM_12 // BMA423 INT2, IO12

#define ACC_INT_1_MASK  GPIO_SEL_14
#define ACC_INT_2_MASK  GPIO_SEL_12

// 确认键按下唤醒 || BMA423抬腕唤醒 || BMA423双击唤醒
#define BTN_PIN_MASK  MENU_BTN_MASK|ACC_INT_1_MASK|ACC_INT_2_MASK // |BACK_BTN_MASK|UP_BTN_MASK|DOWN_BTN_MASK

#define BATT_ADC_PIN GPIO_NUM_32

#define SCL_OLED GPIO_NUM_22
#define SDA_OLED GPIO_NUM_21

#define SCL_1 GPIO_NUM_26 // RTC,BMA用
#define SDA_1 GPIO_NUM_25

#define LED_1 GPIO_NUM_18
#define LED_2 GPIO_NUM_13 // 不能用IO0, 在启动的时候默认是高电平,deepsleep也是高电平

#define BUZZER_PIN GPIO_NUM_2

#define USB_DET GPIO_NUM_34
#define CHARGE_STAT GPIO_NUM_35

// Weather Settings
#define CITY_ID "1796236"  // 1796236 上海  1795565 SHENZHEN City https://openweathermap.org/current#cityid  (在这里可以查询城市代码)

// You can also use LAT,LON for your location instead of CITY_ID, but not both
#define LAT "31.230391" // 上海, Looked up on https://www.latlong.net/
#define LON "121.473701"

// 这个天气接口不好, 最低和最高温度总是一样, 3.0接口还要收费
// http://api.openweathermap.org/data/2.5/weather?id=1796236&lang=en&units=metric&appid=0d1949854101140caad159dc9a23ca86
#ifdef CITY_ID
  #define OPENWEATHERMAP_URL "http://api.openweathermap.org/data/2.5/weather?id={cityID}&lang={lang}&units={units}&appid={apiKey}"  //open weather api using city ID
#else
  #define OPENWEATHERMAP_URL "http://api.openweathermap.org/data/2.5/weather?lat={lat}&lon={lon}&lang={lang}&units={units}&appid={apiKey}"  //open weather api using lat lon
#endif

#define OPENWEATHERMAP_APIKEY "0d1949854101140caad159dc9a23ca86"  //use your own API key :)
#define TEMP_UNIT "metric"                                        // metric = Celsius , imperial = Fahrenheit
#define TEMP_LANG "en"
#define WEATHER_UPDATE_INTERVAL 30  // must be greater than 5, measured in minutes
// NTP Settings
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 3600 * 8  // SHENZHEN UTC+8 = 3800 * 8  https://time.artjoey.com/cn/ (在这里可以查询时差)

// wifi
#define WIFI_AP_TIMEOUT 60
#define WIFI_AP_SSID    "XYWatch AP"

#endif /* CONFIG_H_ */
