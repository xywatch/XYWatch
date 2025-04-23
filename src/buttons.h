/*
 * Project: N|Watch
 * Author: Zak Kemble, contact@zakkemble.co.uk
 * Copyright: (C) 2013 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: http://blog.zakkemble.co.uk/diy-digital-wristwatch/
 */

#ifndef BUTTONS_H_
#define BUTTONS_H_
#include "typedefs.h"

#define UP_BTN_KEY digitalRead(UP_BTN_PIN) // left sw3
#define CONFIRM_BTN_KEY digitalRead(MENU_BTN_PIN) // ok sw2 确认键
#define DOWN_BTN_KEY digitalRead(DOWN_BTN_PIN) // right sw1

typedef enum
{
	BTN_1 = 0,
	BTN_2 = 1,
	BTN_3 = 2,
	BTN_COUNT = 3 // must be last
} btn_t;

void buttons_init(void);
void buttons_update(void);
void buttons_startup(void);
void buttons_shutdown(void);
button_f buttons_setFunc(btn_t, button_f);
void buttons_setFuncs(button_f, button_f, button_f);
// millis_t buttons_pressTime(btn_t);
bool buttons_isActive(void);
void buttons_wake(void);

#endif /* BUTTONS_H_ */
