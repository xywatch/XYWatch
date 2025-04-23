#ifndef GAME_LIFE_H_
#define GAME_LIFE_H_

#if COMPILE_GAME_LIFE

#define GAME_LIFE_NAME STR_GAME_LIFE

//#define PI    3.141592653589793
#define LIFEGAME_WIDTH  128
#define LIFEGAME_HEIGHT 64
#define ALIVE  1
#define DEAD  0

#define BTN_LEFT 1
#define BTN_RIGHT 2
#define BTN_FUNC 3

/*  title  */
void initTitle(void);
bool updateTitle(byte btnType);
void gamelife_drawTitle(void);
void clearHeader(void);

/*  game  */
void initGame(void);
bool updateGame(byte btnType);
void drawGame(void);
void computeAllPointsStatus(void);

void game_life_start(void);

#endif
#endif
