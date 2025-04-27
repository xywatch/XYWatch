#include "common.h"

#define STR_WIFICMDMENU "< Quickstart >"
#define Sync_NTP "Sync NTP"
#define Deep_Sleep "Deep Sleep"
#define Show_Accelerometer "Show Accelerometer"
#define Setup_WiFi "Setup WiFi"
#define Setup_WiFi_BLE "Setup By BLE"
#define Update_Firmware "Update Firmware"
#define StepLog "Step Log"
#define About "About"
#define CMD5_NAME "Back"

#define OPTION_COUNT 8

static void mSelect()
{
    doAction(false); // 执行指令
    // menuData.isOpen = false;  //关闭菜单
}

// 关机
void ShutDown(void)
{
    // display_startCRTAnim(CRTANIM_CLOSE);
    // GPIO_ResetBits(POWER_ON_PORT, POWER_ON_PIN);
}

void sleep_me(void)
{
    OLED_Clear();
    console_log(100, "Sleeping...");
    delay(1000); // 不delay的话, 会直接进入休眠, 按键直接触发中断了
    enterSleep();
}

// log
// extern u8 log_time;
u8 log_time = 5;
static void LogTimeUpdate()
{
    //	battery_updateNow();
    log_time += 2;

    if (log_time > 15)
    {
        log_time = 1;
    }
}

static void itemLoader(byte num)
{
    char buff[20];
    num = 0;

    setMenuOption_P(num++, PSTR(Deep_Sleep), NULL, sleep_me);

    setMenuOption_P(num++, PSTR(Sync_NTP), NULL, showSyncNTP);

    setMenuOption_P(num++, PSTR(Show_Accelerometer), NULL, showAccelerometer);

    setMenuOption_P(num++, PSTR(Setup_WiFi), NULL, showSetupWifi);
    setMenuOption_P(num++, PSTR(Setup_WiFi_BLE), NULL, showBLESetting);

    setMenuOption_P(num++, PSTR(Update_Firmware), NULL, showUpdateFirmware);
    setMenuOption_P(num++, PSTR(StepLog), NULL, showStepLog);
    setMenuOption_P(num++, PSTR(About), NULL, showAbout);
    // setMenuOption_P(num++, buff, NULL, LogTimeUpdate);

    setMenuOption_P(num++, PSTR(CMD5_NAME), NULL, back_to_watchface);
}

void my_menu_open(void)
{
    menuData.isOpen = true; // 打开菜单

    display_setDrawFunc(menu_draw); // 绑定绘制函数为menu_draw

    buttons_setFuncs(menu_up, menu_select, menu_down); // 绑定按键功能函数

    setMenuInfo(OPTION_COUNT, MENU_TYPE_STR, PSTR(STR_WIFICMDMENU)); // 获取当前菜单信息（选项个数，菜单类型是文字还是图标）
    setMenuFuncs(MENUFUNC_NEXT, mSelect, MENUFUNC_PREV, itemLoader); // 绑定菜单的函数,如前进后退选择确认
    showMeThenRun(NULL);
}

// 实现表盘从上往下退出, 然后执行my_menu_open
bool my_menu_open2(void)
{
    if (!animation_active() || animation_movingOn())
    {
        exitMeThenRun(my_menu_open);
    }
    return true;
}
