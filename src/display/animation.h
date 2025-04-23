/*
 * Project: N|Watch
 * Author: Zak Kemble, contact@zakkemble.co.uk
 * Copyright: (C) 2013 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: http://blog.zakkemble.co.uk/diy-digital-wristwatch/
 */

#ifndef ANIMATION_H_
#define ANIMATION_H_
#include "typedefs.h"
#include <stdio.h>

#define ANIM_MOVE_OFF true // ANIM_MOVE_ON动画从下往上  相当于关闭界面
#define ANIM_MOVE_ON false // ANIM_MOVE_ON动画从上往下   相当于开启界面

void animation_init(void);
void animation_update(void);
void animation_start(void (*animOnComplete)(void), bool);
bool animation_active(void);
bool animation_movingOn(void);
byte animation_offsetY(void);
void animation_reset_offsetY(void);
#endif /* ANIMATION_H_ */
