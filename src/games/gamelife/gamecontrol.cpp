#include "common.h"
#include "./gamelife.h"

u8 Display_Temp[LIFEGAME_WIDTH][LIFEGAME_HEIGHT];

uint8_t frameRate;
uint16_t frameCount;
uint8_t eachFrameMillis;
long lastFrameStart;
long nextFrameStart;
bool post_render;
uint8_t lastFrameDurationMs;
// static bool toDraw;
uint16_t Counter;

int NeighborCount(int x, int y) // 判断当前细胞周围存活个数
{
	int i;
	int j;
	int count = 0;
	if (x >= (LIFEGAME_WIDTH - 1) || y >= (LIFEGAME_HEIGHT - 1) || x <= 0 || y <= 0) // 边界处理
	{
		return 0xff;
	}
	for (i = x - 1; i <= x + 1; i++)
	{
		for (j = y - 1; j <= y + 1; j++)
			if (draw_get_point(i, j))
			{
				count++;
			}
	}
	if (draw_get_point(x, y))
	{
		count--;
	}

	return count;
}

// 显示到屏幕上
void flushToScreen(void)
{
	int i, j;
	for (i = 0; i < LIFEGAME_WIDTH; i++)
	{
		for (j = 0; j < LIFEGAME_HEIGHT; j++)
		{
			if (Display_Temp[i][j] == ALIVE)
			{
				draw_set_point(i, j, 1);
			}
			else
			{
				draw_set_point(i, j, 0);
			}
		}
	}
}

void initGame(void)
{
	int i, j;
	Counter = 0;
	for (i = 0; i < LIFEGAME_WIDTH; i++)
	{
		for (j = 0; j < LIFEGAME_HEIGHT; j++)
		{
			Display_Temp[i][j] = 0;
		}
	}
}

// 计算所有点的状态
void computeAllPointsStatus(void) {
	int i, j;
	for (i = 0; i < LIFEGAME_WIDTH; i++)
	{
		for (j = 0; j < LIFEGAME_HEIGHT; j++)
		{
			if (draw_get_point(i, j))
			{
				switch (NeighborCount(i, j))
				{
				case 0:
					Display_Temp[i][j] = DEAD;
					break;

				case 1:
					Display_Temp[i][j] = DEAD;
					break;

				case 2:
					Display_Temp[i][j] = ALIVE;
					break;

				case 3:
					Display_Temp[i][j] = ALIVE;
					break;

				case 4:
					Display_Temp[i][j] = DEAD;
					break;

				case 5:
					Display_Temp[i][j] = DEAD;
					break;

				case 6:
					Display_Temp[i][j] = DEAD;
					break;

				case 7:
					Display_Temp[i][j] = DEAD;
					break;

				case 8:
					Display_Temp[i][j] = DEAD;
					break;

				default:
					Display_Temp[i][j] = DEAD;
					break;
				}

				// Display_Temp[i][j] = ALIVE;
			}
			else
			{
				switch (NeighborCount(i, j))
				{
				case 0:
					Display_Temp[i][j] = DEAD;
					break;

				case 1:
					Display_Temp[i][j] = DEAD;
					break;

				case 2:
					Display_Temp[i][j] = DEAD;
					break;

				case 3:
					Display_Temp[i][j] = ALIVE;
					break;

				case 4:
					Display_Temp[i][j] = DEAD;
					break;

				case 5:
					Display_Temp[i][j] = DEAD;
					break;

				case 6:
					Display_Temp[i][j] = DEAD;
					break;

				case 7:
					Display_Temp[i][j] = DEAD;
					break;

				case 8:
					Display_Temp[i][j] = DEAD;
					break;

				default:
					Display_Temp[i][j] = DEAD;
					break;
				}

				// Display_Temp[i][j] = DEAD;
			}
		}
	}
}

bool updateGame(byte btnType)
{
	bool ret;
	if (btnType == BTN_FUNC)
	{
		ret = true;
	}
	else
	{
		ret = false;
	}
	return ret;
}

void drawGame(void)
{
	flushToScreen();

	Counter++;
	if (Counter % 3 == 0) {
		// 重新再计算状态, 供下一次用, 刷新很快, 怎么慢点? 每5次刷一次, 不重新计算则表示还是之前的状态
		computeAllPointsStatus();
	}
	Counter = Counter % 3;
}
