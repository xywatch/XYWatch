#include "common.h"
extern bool keep_on;
extern float VBAT;

static display_t draw() {
    u8 y = 0;
    draw_string(PSTR("User: " USER_NAME), false, 0, y);
    // draw_string(PSTR("FW: " FW_VERSION), false, 0, y += 8);
    draw_string(PSTR("HW: " HW_VERSION), false, 0, y += 8);
    // y += 10;
    // draw_fill_screen(0, y, 127, y, 1);
    // y += 2;
    char buf[49];
    sprintf(buf, "Voltage: %0.2f", VBAT);
    draw_string(buf, false, 0, y += 8);

    draw_string(PSTR("Wifi: "), false, 0, y += 8);
    draw_string(appConfig.wifiName, false, 0, y += 8);
    draw_string(appConfig.wifiPassword, false, 0, y += 8);

    return DISPLAY_DONE;
}

static bool btnExit()
{
  keep_on = false;
  back_to_watchface();
  return true;
}

void showAbout(void)
{
  getBatteryVoltage();
  display_setDrawFunc(draw);
  buttons_setFuncs(btnExit, btnExit, btnExit);
  showMeThenRun(NULL);
  keep_on = true;
}