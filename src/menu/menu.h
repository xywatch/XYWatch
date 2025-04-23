/*
 * Project: N|Watch
 * Author: Zak Kemble, contact@zakkemble.co.uk
 * Copyright: (C) 2013 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: http://blog.zakkemble.co.uk/diy-digital-wristwatch/
 */

#ifndef MENU_H_
#define MENU_H_

#include "typedefs.h"

bool menu_select(void);
bool menu_down(void);
bool menu_up(void);
bool menu_isOpen(void);
void menu_close(void);


#define MAX_MENU_ITEMS 7 // max number of items to show on screen (can have more, but they won't be shown)

#define MENUFUNC_NEXT nextOption
#define MENUFUNC_PREV prevOption

typedef struct
{
	byte now;
	byte val;
} s_menuNowSetting;

extern s_menuNowSetting setting;
extern menu_s menuData;
extern const char menuBack[];

// #define setMenuOption(a, b, c, d) setMenuOption2(a, b, c, d)
// #define setMenuOption_P(a, b, c, d) setMenuOption2_P(a, b, c, d)
// #define setMenuInfo(a, b, c, d, e, f, g) setMenuInfo2(a, b, c, d, e, f)

display_t menu_draw(void);
void addBackOption(void);
void back(void);
void exitMeThenRun(menu_f);
void showMeThenRun(menu_f);
void setMenuInfo(byte, menu_type_t, const char *);
void setMenuFuncs(menu_f, menu_f, menu_f, itemLoader_f);
void setMenuOption(byte, const char *, const byte *, menu_f);
void setMenuOption_P(byte, const char *, const byte *, menu_f);
void nextOption(void);
void prevOption(void);
void doAction(bool);
void setPrevMenuOpen(prev_menu_s *, menu_f);
void setPrevMenuExit(prev_menu_s *);
bool exitSelected(void);
void back_to_watchface(void);
// void do10sStuff(byte*, byte);
// void do1sStuff(byte*, byte, byte, byte);

#endif /* MENU_H_ */
