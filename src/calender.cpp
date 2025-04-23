#include "common.h"

#if COMPILE_CALENDAR

static bool btnLeft(void);
static bool btnRight(void);
static bool btnExit(void);
static display_t draw(void);
static int is_leap_year(int year);
static int get_week_day(int year, int month, int day, int *month_num);

int curDay = 0;
int curYear = 0;
int curMonth = 0;

void calendarOpen()
{
    // menu_close();
    curYear = timeDate.date.year + 2000;
    curMonth = timeDate.date.month; // 0-11
    curDay = timeDate.date.date;    // 1-31

    display_setDrawFunc(draw);
    buttons_setFuncs(btnRight, btnExit, btnLeft);
    showMeThenRun(NULL); // 打开动画动画过度
}

static bool btnLeft()
{
    curDay--;
    return true;
}

static bool btnRight()
{
    curDay++;
    return true;
}

static bool btnExit()
{
    // animation_start(display_load, ANIM_MOVE_OFF);
    exitMeThenRun(back);
    return true;
}

static int is_leap_year(int year)
{
    if (year % 100 == 0 && year % 400 == 0)
    {
        return 1;
    }
    else if (year % 4 == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

// month_num本月有多少天
// month = 1-12
static int get_week_day(int year, int month, int day, int *month_num)
{
    // 算月天
    if (month == 2)
    {
        if (is_leap_year(year) == 1)
        {
            *month_num = 29;
        }
        else
        {
            *month_num = 28;
        }
    }
    else if (month == 4 || month == 6 || month == 9 || month == 11)
    {
        *month_num = 30;
    }
    else
    {
        *month_num = 31;
    }

    // 算星期
    if (month == 1 || month == 2)
    {
        year -= 1;
        month += 12;
    }

    // week    0->周一    6->周日
    return (day + 2 * month + 3 * (month + 1) / 5 + year + year / 4 - year / 100 + year / 400) % 7;
}

byte getItemPosX(byte colNum)
{
    byte x = 5 * 2 * colNum + colNum * 2; // 6 * 2 * 7 (128-42*2)/6=7
    return x;
}

// month从0开始, day从1开始
void showCalendar(int year, int month, int day)
{
    int month_num = 0; // 当月有多少天
    int week_day = 0;  // 第一周是从周几开始
    u8 curDayWeek = 0;

    // 得到本月1号的星期
    week_day = get_week_day(year, month + 1, 1, &month_num);

    if (day > month_num)
    {
        day = 1;
        month++;

        if (month >= 12)
        {
            month = 0;
            year++;
        }

        week_day = get_week_day(year, month + 1, 1, &month_num);
    }
    else if (day <= 0)
    {
        month--;

        if (month < 0)
        {
            month = 11;
            year--;
        }

        week_day = get_week_day(year, month + 1, 1, &month_num);
        day = month_num;
    }

    curDay = day;
    curYear = year;
    curMonth = month;

    // 显示日历
    int i = 0;
    int j = 0;
    int count = 1;
    bool isInvert = false;
    char temp[4] = {0};

    // 5行7列
    u8 lineNum = 6;
    u8 colNum = 7;

    // 总高 头 10 + 9 * 6 = 64

    // 头 画星期
    for (j = 0; j < colNum; j++)
    {
        temp[0] = ' ';
        temp[1] = j + 1 + '0';
        temp[2] = '\0';
        byte x = getItemPosX(j);
        draw_string_min(temp, false, x, 0);
    }
    // 再画一条横线
    draw_fill_screen(0, 9, 85, 9, 1);

    byte startY = 10;

    for (i = 0; i < lineNum; i++)
    {
        for (j = 0; j < colNum; j++)
        {
            if (j < week_day && i == 0)
            { // 打印空格 第一行, 不是本月的, 显示为空
                temp[0] = ' ';
                temp[1] = ' ';
                temp[2] = '\0';
            }
            else if (count < 10)
            {
                temp[0] = ' ';
                temp[1] = count + '0';
                temp[2] = '\0';
                count++;
            }
            else
            {
                temp[0] = count / 10 + '0';
                temp[1] = count % 10 + '0';
                temp[2] = '\0';
                count++;
            }

            if (count - 1 > month_num)
            {
                temp[0] = ' ';
                temp[1] = ' ';
                temp[2] = '\0';
                // break;
            }

            isInvert = 0;

            if (day + 1 == count && count >= 1)
            {
                curDayWeek = j;
                isInvert = 1;
            }

            // 字体宽 5 * 8
            // 高8 * 5 = 40 8 * 6 = 48 8 * 7 = 56
            // 高12 * 5 = 60

            // if (count >= 1) {
            // 最后一个数字的位置 = 12*7=84,右边还有128-84=44
            byte x = getItemPosX(j); // 5 * 2 * j + j * 2;

            byte y = startY + i * 9; // 8+1
            // ssd1306_display_string(x, y, temp, 12, chMode);
            draw_string_min(temp, isInvert, x, y);
        }
    }

    // 画一条竖线
    draw_fill_screen(85, 0, 85, 63, 1);

    // 在右侧区域写日期, 星期

    // 2012-12-12
    // unsigned char monthDay[6] = {"00-00"};
    char monthDay[6];
    char yearStr[5];
    u8 realMonth = month + 1;

    byte startX = 86;
    byte rightWidth = FRAME_WIDTH - startX;

    sprintf(monthDay, "%02d-%02d", realMonth, day);
    sprintf(yearStr, "%04d", year);

    draw_string(yearStr, false, startX + (rightWidth - 4 * 6) / 2, 12);
    draw_string(monthDay, false, startX + (rightWidth - 5 * 6) / 2, 12 + 12);

    char weekStr[BUFFSIZE_STR_DAYS];
    strcpy(weekStr, days[curDayWeek]);
    draw_string(weekStr, false, startX + (rightWidth - 3 * 6) / 2, 12 + 12 + 12);
}

static display_t draw()
{
    // showCalendar(2022, 3, 2);
    showCalendar(curYear, curMonth, curDay);
    return DISPLAY_BUSY;
}

#endif
