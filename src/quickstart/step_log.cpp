#include "common.h"
extern bool keep_on;

extern BMA423 sensor;
RTC_DATA_ATTR stepLogData_s stepLogs[STEP_LOG_COUNT]; // 0 是昨天, 1是前天

static display_t draw()
{
  char p[6], i;
  draw_string("Date   Step", NOINVERT, 0, 0);

  for (i = 0; i < STEP_LOG_COUNT; i++)
  {
    sprintf((char *)p, "%02d-%02d", stepLogs[i].month + 1, stepLogs[i].date); // time
    draw_string(p, NOINVERT, 0, 9 + i * 9);
    sprintf((char *)p, "%d", stepLogs[i].stepCount);
    draw_string(p, NOINVERT, 50, 9 + i * 9);
  }

  return DISPLAY_DONE;
}

static bool btnExit()
{
  keep_on = false;
  back_to_watchface();
  return true;
}

void addStepLog(void)
{
  byte year = timeDate.date.year;         // 以2000为基准
  byte month = (byte)timeDate.date.month; // 0-11, 月份, 0:一月, 11:十二月
  byte date = timeDate.date.date;         // 1-31, 日期

  // 计算上一天
  if (date == 1)
  {
    // 前一个月
    if (month == 0)
    {
      // 前一年最后一天
      year--;
      month = 11; // 十二月
      date = 31;  // 十二月固定31天
    }
    else
    {
      // 上一个月
      month--;
      // 计算上个月有多少天
      date = time_monthDayCount((month_t)month, year);
    }
  }
  else
  {
    // 前一天
    date--;
  }

  // 数据后移 0->1
  // 使用memmove代替memcpy，以安全处理重叠的内存区域
  memmove(&stepLogs[1], &stepLogs[0], (STEP_LOG_COUNT-1) * sizeof(stepLogData_s));

  stepLogs[0].year = year;
  stepLogs[0].month = (month_t)month;
  stepLogs[0].date = date;
  stepLogs[0].stepCount = sensor.getCounter();

  appconfig_save_step_log();
}

void showStepLog(void)
{
  display_setDrawFunc(draw);
  buttons_setFuncs(btnExit, btnExit, btnExit);
  showMeThenRun(NULL);
  keep_on = true;
}