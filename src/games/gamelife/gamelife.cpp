#include "common.h"
#include "./gamelife.h"

static bool btnExit(void);
static bool btnRight(void);
static bool btnLeft(void);
display_t game_life_draw(void);

/*  Defines  */

enum MODE
{
	TITLE_MODE,
	GAME_MODE
};

static enum MODE mode = TITLE_MODE;
bool isDone;
byte btnType;

// 中间
static bool btnExit()
{
		btnType = BTN_FUNC;

    // animation_start(display_load, ANIM_MOVE_OFF);
    return true;
}

// 右
static bool btnLeft()
{
		btnType = BTN_LEFT;
    return true;
}

// 右
static bool btnRight()
{
		btnType = BTN_RIGHT;
    return true;
}

// 开始只调用一次
void game_life_start()
{
    menu_close();

    display_setDrawFunc(game_life_draw);
    buttons_setFuncs(btnRight, btnExit, btnLeft);
		
		mode = TITLE_MODE;
		initTitle();
}

// 画每一帧都调用
display_t game_life_draw()
{
		// title mode
		if (mode == TITLE_MODE) {
			bool isDone = updateTitle(btnType);
			gamelife_drawTitle();
			// 变成game模式
			if (isDone)
			{
				mode = GAME_MODE;
				initGame();
				// 先清空头部title, 再备份
				clearHeader();
				// 备份下上一次的状态
				computeAllPointsStatus();
			}
		}
		// game mode
		else {
			bool isDone = updateGame(btnType); // 没什么用了
			drawGame();
			// 变成title mode
			if (isDone) {
				mode = TITLE_MODE;
				initTitle();
			}
			else if (btnType == BTN_RIGHT) {
				// 关闭
				exitMeThenRun(display_load);
			}
		}

		// 还原btn
		btnType = 0;

		return DISPLAY_BUSY;  //返回屏幕刷新忙
}
