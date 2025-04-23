#ifndef COMMON_H_
#define COMMON_H_

#include <Arduino.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "config/config.h"
#include "util.h"
#include "typedefs.h"
#include "resources/resources.h"
#include "config/appconfig.h"
#include "display/oled.h"
#include "display/animation.h"
#include "display/display.h"
#include "display/draw.h"
#include "watchface.h"
#include "buttons.h"
#include "time.h"
#include "led/led.h"

#include "stopwatch.h"
#include "calendar.h"
#include "torch.h"
#include "alarm/alarms.h"
#include "alarm/alarm.h"
#include "pwrmgr.h"

#include "settings/settings.h"
#include "settings/timedate.h"
#include "settings/display.h"
#include "settings/sleep.h"
#include "settings/sound.h"
#include "settings/diag.h"
#include "settings/ble.h"

#include "tune/beep.h"
#include "tune/tune.h"
#include "tune/tunes.h"
#include "tune/tunemaker.h"

#include "games/games.h"
#include "games/game1.h"
#include "games/game2.h"
#include "games/game3.h"
#include "games/game_snake.h"
#include "games/gamelife/gamelife.h"

#include "menu/menu.h"
#include "menu/menu_main.h"

#include "quickstart/my_menu.h"
#include "quickstart/about.h"

#include "rtc/rtc.h"

#include "bma423/bma.h"
#include "bma423/bma423_watch.h"

#include "display/console.h"

#include "ntp/ntp.h"
#include "wifiutil.h"
#include "bleota.h"
#include "weather/weather.h"

#endif /* COMMON_H_ */
