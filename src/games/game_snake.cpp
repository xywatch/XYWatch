#include "common.h"

#if COMPILE_GAME_SNAKE

#define UP 3
#define DOWN 4
#define LEFT 1
#define RIGHT 2

// 地图内的点类型
#define FOOD 1
#define BODY 2
#define BLANK 0

const byte startPosX = 2;    // 地图左上X 不包括围墙
const byte startPosY = 2;    // 地图左上Y 不包括围墙
const byte snakeWidth = 31;  // * 4 = 124 不包括围墙
const byte snakeHeight = 13; // * 4 = 52 不包括围墙

const byte wallSize = 1; // 墙厚度, 墙在map的四周

byte game_snake_map[31][13] = {0}; // 地图大小  x-y
int snake_Grid[128][2] = {0};            // 蛇坐标最多128个点
const byte initSnakeLength = 3;
byte snakeLength;
byte snakeScore;
byte snakeDirection; // 蛇的方向
byte foodNum = 0;
bool snakeEated = false;
bool gameIsOver = false;
bool gameIsPaused = false;

byte refreshHz = 10; // 每10次渲染一次, 值越小, 速度越快
byte renderI = 0;

void Create_Wall(void);                    // 初始化地图边界
void Move(void);                           // 移动
void Paint_Map(byte x, byte y, byte mode); // 绘制地图
void Paint_Map2(void);
void Paint_Food(byte x, byte y);  // 绘制食物
void Paint_Body(byte x, byte y);  // 绘制身体
void drawSnake(void);             // 画蛇
void Snake_Init(void);            // 蛇及食物初始化
void Paint_Clean(byte x, byte y); // 清除点
void Grid_Bound(void);            // 坐标限定
bool GameOver(void);              // 游戏结束
void Gen_Food(void);              // 生成食物
void Eat_Food(void);              // 吃食物

static bool btnExit(void);
static bool btnRight(void);
static bool btnLeft(void);
display_t game_snake_draw(void);

/*
---------------------x 32
|
|
|
y 12

-2 map 四周框
-1 food
2 body
0 空区

每个点对应到屏幕是4x4像素
围墙是 4px ..**
body *..*
food .**.
*/

void Snake_Refresh() // 界面刷新
{
    byte i, j, temp;

    for (i = 0; i < snakeWidth; i++)
    {
        for (j = 0; j < snakeHeight; j++)
        {
            temp = game_snake_map[i][j];

            if (temp == BODY)
            {
                Paint_Body(i, j);
            }
            // else if (temp == -2 || temp == -3 || temp == -4 || temp == -5)
            // {
            //  Paint_Map(i, j, temp);
            // }
            else if (temp == FOOD)
            {
                Paint_Food(i, j);
            }
            else if (temp == BLANK)
            {
                Paint_Clean(i, j);
            }
        }
    }

    // Paint_Map2();

    // 创建wall
    Create_Wall();
}

/*
绘制食物
*/
void Paint_Food(byte x, byte y)
{
    byte i, j;

    for (i = 4 * y; i < 4 * y + 4; i++)
    {
        for (j = 4 * x; j < 4 * x + 4; j++)
        {
            // 只要中间2像素
            if (i == 4 * y + 1 || i == 4 * y + 2)
            {
                draw_set_point(j + startPosX, i + startPosY, 1);
            }

            if (j == 4 * x + 1 || j == 4 * x + 2)
            {
                draw_set_point(j + startPosX, i + startPosY, 1);
            }
        }
    }
}

// 绘制snake身体
void Paint_Body(byte x, byte y)
{
    byte i, j;

    for (i = 4 * y; i < 4 * y + 4; i++)
    {
        for (j = 4 * x; j < 4 * x + 4; j++)
        {
            // 只要*..* 中间留空
            if (i == 4 * y || i == 4 * y + 3)
            {
                draw_set_point(j + startPosX, i + startPosY, 1);
            }

            if (j == 4 * x || j == 4 * x + 3)
            {
                draw_set_point(j + startPosX, i + startPosY, 1);
            }
        }
    }
}

// 清除点
void Paint_Clean(byte x, byte y)
{
    byte i, j;

    for (i = 4 * y; i < 4 * y + 4; i++)
    {
        for (j = 4 * x; j < 4 * x + 4; j++)
        {
            draw_set_point(j + startPosX, i + startPosY, 0);
        }
    }
}

// 四周围墙
// 只画1像素的墙
void Create_Wall()
{
    byte leftX = startPosX - wallSize;
    byte topY = startPosY - wallSize;
    byte rightX = snakeWidth * 4 + wallSize;
    byte bottomY = snakeHeight * 4 + wallSize;

    // 上
    draw_fill_screen(leftX, topY, rightX, topY + wallSize - 1, 1);
    // 下
    draw_fill_screen(leftX, bottomY, rightX, bottomY + wallSize - 1, 1);

    // 左
    draw_fill_screen(leftX, topY, leftX + wallSize - 1, bottomY, 1);
    // 右
    draw_fill_screen(rightX, topY, rightX + wallSize - 1, bottomY, 1);
}

void Snake_Init() // 蛇及食物初始化
{
    memset(game_snake_map, 0, sizeof(game_snake_map));
    memset(snake_Grid, 0, sizeof(snake_Grid));
    snakeLength = initSnakeLength; // 开始长度为5个
    snakeScore = 0;
    foodNum = 0;
    gameIsOver = false;
    gameIsPaused = false;
    snake_Grid[0][0] = 7; // x坐标,蛇头坐标
    snake_Grid[0][1] = 5; // y坐标

    for (byte i = 1; i < initSnakeLength; i++)
    {
        snake_Grid[i][0] = snake_Grid[0][0] - i;
        snake_Grid[i][1] = snake_Grid[0][1]; // 给刚开始的蛇身几个初始坐标
    }

    snakeDirection = RIGHT;

    // 投食
    Gen_Food();
}

/*
定时
Move();
Eat_Food();
drawSnake();
*/

/*
移动
1. 每移动一次就是清除尾巴
2. 是否吃了, 吃了长度变长
3. 后一点坐标变成前一点的坐标
4. 第一点根据方向变
---
 ---
*/
void Move()
{
    game_snake_map[snake_Grid[snakeLength - 1][0]][snake_Grid[snakeLength - 1][1]] = BLANK; // 清除尾巴

    if (snakeEated)
    { // 如果吃到了食物
        snakeLength++;
        snakeEated = false; // 设置为false，不然无限变长
    }

    byte i;

    for (i = snakeLength - 1; i > 0; i--)
    { // 从尾巴开始，每一个点的位置等于它前面一个点的位置
        snake_Grid[i][0] = snake_Grid[i - 1][0];
        snake_Grid[i][1] = snake_Grid[i - 1][1];
    }

    switch (snakeDirection)
    {
    case UP:
        snake_Grid[0][1]--;
        break;

    case DOWN:
        snake_Grid[0][1]++;
        break;

    case LEFT:
        snake_Grid[0][0]--;
        break;

    case RIGHT:
        snake_Grid[0][0]++;
        break;
    }

    Grid_Bound(); // 坐标限定
}

// 坐标限定, 是否碰到墙了
void Grid_Bound()
{
    // 到最右了
    if (snake_Grid[0][0] == snakeWidth)
    {
        snake_Grid[0][0] = 0;
    }
    // 到最左了
    else if (snake_Grid[0][0] == -1)
    {
        snake_Grid[0][0] = snakeWidth - 1;
    }
    // 到最底了
    else if (snake_Grid[0][1] == snakeHeight)
    {
        snake_Grid[0][1] = 0;
    }
    // 到顶了
    else if (snake_Grid[0][1] == -1)
    {
        snake_Grid[0][1] = snakeHeight - 1;
    }
}

bool isBlank(byte i, byte j) // 检查地图空位
{
    if (game_snake_map[i][j] != 0)
    {
        return false;
    }

    return true; // 是空位就返回1
}

void Gen_One_Food()
{
    byte i, j;

    do
    {
        i = rand() % snakeWidth; // 生成0~H-1之间的一个数
        j = rand() % snakeHeight;
    } while (!isBlank(i, j));

    game_snake_map[i][j] = FOOD; // 画出食物
    foodNum++;
}

// 生成食物
void Gen_Food()
{
    // 如果有, 随机不再生成
    if (foodNum > 0 && rand() % 2)
    {
        return;
    }

    Gen_One_Food();

    if (foodNum >= 5)
    {
        return;
    }

    // 随机再显示一个
    if (rand() % 2)
    {
        Gen_One_Food();
    }
}

// 吃食物
void Eat_Food()
{
    if (game_snake_map[snake_Grid[0][0]][snake_Grid[0][1]] == FOOD)
    {                      // 如果蛇头碰到食物，就重新投放食物，并且把食物点重置为0
        snakeEated = true; // 标记已经吃到食物
        snakeScore++;
        foodNum--;
        Gen_Food();
        game_snake_map[snake_Grid[0][0]][snake_Grid[0][1]] = BLANK; // 去掉食物
    }
}

// 画蛇
void drawSnake()
{
    byte i, x, y;

    for (i = 0; i < snakeLength; i++)
    {
        x = snake_Grid[i][0];
        y = snake_Grid[i][1];
        game_snake_map[x][y] = BODY;
    }
}

bool GameOver() // 游戏结束
{
    bool isGameOver = false;
    byte sx = snake_Grid[0][0], sy = snake_Grid[0][1], i; // 蛇头坐标

    for (i = 1; i < snakeLength; i++)
    { // 判断有没有吃到自己
        if (snake_Grid[i][0] == sx && snake_Grid[i][1] == sy)
        {
            isGameOver = true;
        }
    }

    return isGameOver;
}

// 游戏开始
void game_snake_start()
{
    menu_close();

    srand(millis());
    display_setDrawFunc(game_snake_draw);
    buttons_setFuncs(btnRight, btnExit, btnLeft);

    Snake_Init();
}

// 游戏退出
static bool btnExit()
{
    // 在没over时, 按中键就暂停
    if (!gameIsPaused && !gameIsOver)
    {
        gameIsPaused = true;
        return true;
    }

    exitMeThenRun(display_load);
    return true;
}

// 向右移动 + 向下
static bool btnRight()
{
    if (gameIsPaused)
    {
        gameIsPaused = false;
        return true;
    }

    if (gameIsOver)
    {
        Snake_Init();
        return true;
    }

    if (snakeDirection == RIGHT || snakeDirection == LEFT)
    {
        snakeDirection = DOWN;
    }
    else
    {
        snakeDirection = RIGHT;
    }

    return true;
}

// 向左移动
static bool btnLeft()
{
    if (gameIsPaused)
    {
        gameIsPaused = false;
        return true;
    }

    if (gameIsOver)
    {
        Snake_Init();
        return true;
    }

    if (snakeDirection == LEFT || snakeDirection == RIGHT)
    {
        snakeDirection = UP;
    }
    else
    {
        snakeDirection = LEFT;
    }

    return true;
}

// 游戏绘图
display_t game_snake_draw()
{
    byte score = snakeLength - initSnakeLength;
    refreshHz = 10 - score / 5; // 5分速度变成9, 10分速度变成8

    if (refreshHz < 2)
    {
        refreshHz = 2;
    }

    gameIsOver = GameOver();

    if (!gameIsOver && !gameIsPaused)
    {
        if (renderI % refreshHz == 0)
        {
            Move();
            Eat_Food();
            drawSnake();
        }
    }

    Snake_Refresh();

    // 分数
    char scoreStr[12] = {""};
    sprintf(scoreStr, "%s%d", "Score:", score);
    draw_string_center(scoreStr, false, 0, FRAME_WIDTH, FRAME_HEIGHT - 8);

    if (gameIsOver)
    {
        draw_string_center("GAME OVER!", false, 0, FRAME_WIDTH, (FRAME_HEIGHT - 8) / 2);
        // draw_string_center(scoreStr, false, 0, FRAME_WIDTH, (FRAME_HEIGHT-8)/2 + 10);
    }
    else if (gameIsPaused)
    {
        draw_string_center("GAME PAUSE!", false, 0, FRAME_WIDTH, (FRAME_HEIGHT - 8) / 2);
    }

    renderI++;
    renderI = renderI % refreshHz;
    return DISPLAY_BUSY; // 返回屏幕刷新忙
}

// 没用

/*
绘制地图 4px
  -2
-4  -5
  -3
void Paint_Map(byte x, byte y, byte mode)
{
    byte i, j;

    for (j = 4 * x; j < 4 * x + 4; j++)
    {
        for (i = 4 * y; i < 4 * y + 4; i++)
        {
            // 只要后面2像素
            // 上
            if (mode == -2) {
                if (i == 4 * y + 2 || i == 4 * y + 3)
                {
                    draw_set_point(j + startPosX, i + startPosY, 1);
                }
            }
            // 只要上面2像素
            // 下
            else if (mode == -3) {
                if (i == 4 * y + 0 || i == 4 * y + 1)
                {
                    draw_set_point(j + startPosX, i + startPosY, 1);
                }
            }
            // 只要右面2像素
            // 左
            else if (mode == -4) {
                if (j == 4 * x + 2 || j == 4 * x + 3)
                {
                    draw_set_point(j + startPosX, i + startPosY, 1);
                }
            }
            // 只要左面2像素
            // 右
            else if (mode == -5) {
                if (j == 4 * x + 0 || j == 4 * x + 1)
                {
                    draw_set_point(j + startPosX, i + startPosY, 1);
                }
            }
        }
    }
}

*/

#endif
