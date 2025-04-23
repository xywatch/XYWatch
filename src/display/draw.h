/*
 * Project: N|Watch
 * Author: Zak Kemble, contact@zakkemble.co.uk
 * Copyright: (C) 2013 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: http://blog.zakkemble.co.uk/diy-digital-wristwatch/
 */

#ifndef DRAW_H_
#define DRAW_H_
#include "typedefs.h"

typedef unsigned char u8;

void draw_string_P(const char *, bool, byte, byte);
void draw_string(char *, bool, byte, byte);
void draw_string_center(char *string, bool invert, byte fromX, byte toX, byte y);
void draw_string_min(char *string, bool invert, byte x, byte y);
// void draw_string_time(char*, bool, byte, byte);
void draw_bitmap(byte x, byte yy, const byte *bitmap, byte w, byte h, bool invert, byte offsetY);
void draw_bitmap2(byte x, byte y, const uint8_t *bitmap, byte w, byte h, byte color);
void draw_clearArea(byte, byte, byte); //, byte);
void draw_end(void);
void draw_fill_screen(byte chXpos1, byte chYpos1, byte chXpos2, byte chYpos2, byte chDot);
void draw_set_point(byte chXpos, byte chYpos, byte chPoint);
bool draw_get_point(byte chXpos, byte chYpos);

#endif /* DRAW_H_ */
