/*
 * Project: N|Watch
 * Author: Zak Kemble, contact@zakkemble.co.uk
 * Copyright: (C) 2013 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: http://blog.zakkemble.co.uk/diy-digital-wristwatch/
 */

#include "common.h"

// Frame rate when stuff is happening
// If this is too low then animations will be jerky
// Animations are also frame rate based instead of time based, adjusting frame rate will also effect animation speed
#define FRAME_RATE 60

// Frame rate when nothing is going on
// If this is too low then the interface will seem a bit slow to respond
#define FRAME_RATE_LOW 25

#define FRAME_RATE_MS ((byte)(1000 / FRAME_RATE))
#define FRAME_RATE_LOW_MS ((byte)(1000 / FRAME_RATE_LOW))

static draw_f drawFunc; // 绘制屏幕的函数
static display_f func; // 打开watch face 函数

byte MY_FPS = FRAME_RATE; // 全局动画帧率控制

typedef struct
{
    byte height; // 动画高度
    bool closing; // 开机 或 关机动画
    bool doingLine; // 是否正在执行动画
    byte lineWidth; // 动画宽度
    byte lineClosing; // 是否正在关闭动画
    bool active; // 动画是否正在执行
} crt_anim_s;

static crt_anim_s crt_anim;

static void crt_animation(void);

bool display_is_ani_active()
{
    return crt_anim.active || animation_active();
}

void display_set(display_f faceFunc)
{
    func = faceFunc;
}

// 打开watch face 函数
void display_load()
{
    if (func != NULL)
    {
        func();
    }
}

// 设置绘制屏幕的函数
draw_f display_setDrawFunc(draw_f func)
{
    draw_f old = drawFunc; // 保存旧的绘制屏幕的函数
    drawFunc = func; // 设置新的绘制屏幕的函数
    return old; // 返回旧的绘制屏幕的函数
}

// 有些界面不需要刷新屏幕, 比如NTP同步界面, 所以需要禁用刷新屏幕
bool display_enabled = true;
void display_update_disable()
{
    display_enabled = false;
}
void display_update_enable()
{
    display_enabled = true;
}

bool display_is_enabled() 
{
    return display_enabled;
}   

// 刷新屏幕
// main.c里会一直调用, 所以会一直刷新屏幕
/*
根据系统状态动态调整帧率
空闲时使用低帧率（25fps）节省资源
有动画时使用高帧率（60fps）保证流畅
这个函数是显示系统的核心更新函数，它：
1. 控制显示刷新频率
2. 管理动画状态
3. 处理绘制操作
4. 实现CRT动画效果
5. 提供FPS显示功能
6. 动态调整帧率以平衡性能和功耗
这个函数在主循环中被频繁调用，确保显示内容能够及时更新，同时通过帧率控制避免过度消耗系统资源。
*/
void display_update()
{
    if (!display_enabled)
    {
        // Serial.println("display_update disabled");
        return;
    }

    static millis8_t lastDraw; // 上一次刷新屏幕的时间
    static byte fpsMs, fpsms;  // 刷新屏幕的间隔时间

    // Limit frame rate
    millis8_t now = millis();

    // 如果当前时间与上一次刷新屏幕的时间差小于刷新屏幕的间隔时间, 则不刷新屏幕
    if ((millis8_t)(now - lastDraw) < fpsMs)
    {
        // pwrmgr_setState(PWR_ACTIVE_DISPLAY, PWR_STATE_IDLE);
        return;
    }

    fpsms = now - lastDraw;
    lastDraw = now;

    // debugPin_draw(HIGH);

    display_t busy = DISPLAY_DONE;

    // 更新动画
    animation_update();

    // 绘制屏幕
    if (drawFunc != NULL && (crt_anim.active || (!crt_anim.active && !crt_anim.closing)))
    {
        busy = drawFunc();
    }

    // 执行开屏和关屏动画
    if (crt_anim.active)
    {
        crt_animation();
    }

    // 如果开屏和关屏动画正在执行, 则设置屏幕刷新状态为刷新中
    if (crt_anim.active || animation_active())
    {
        busy = DISPLAY_BUSY;
    }

    if (appConfig.showFPS)
    {
        // Work out & draw FPS, add 2ms (actually 2.31ms) for time it takes to send to OLED, clear buffer etc
        // This is only approximate
        // millis8_t end = millis() + 1;
        char buff[5];
        sprintf_P(buff, PSTR("%u"), (uint)(1000 / fpsms));
        //	draw_string(buff,false,107,56);
        draw_string(buff, false, 100, 56);
    }

    // End drawing, send to OLED
    draw_end();

    // 决定帧率
    // 如果屏幕刷新完成, 则设置帧率为低帧率
    if (busy == DISPLAY_DONE)
    {
        //	pwrmgr_setState(PWR_ACTIVE_DISPLAY, PWR_STATE_NONE);
        fpsMs = FRAME_RATE_LOW_MS;
    }
    // 如果屏幕刷新中, 则设置帧率为高帧率
    else
    {
        //	pwrmgr_setState(PWR_ACTIVE_DISPLAY, PWR_STATE_IDLE);
        fpsMs = (byte)(1000 / MY_FPS);
        // #if COMPILE_GAME1
        //     if (drawFunc == game1_draw)
        //       fpsMs <<= 1;
        // #endif
    }
}

void display_startCRTAnim(crtAnim_t open)
{
    if (!appConfig.animations)
    {
        crt_anim.active = false;
        return;
    }

    // 开屏动画
    // 中间往上和往下展开
    if (open == CRTANIM_OPEN)
    {
        crt_anim.closing = false;    // 开屏动画
        crt_anim.doingLine = true;   // 直接开始扫描线效果
        crt_anim.height = FRAME_HEIGHT / 2;  // 高度从屏幕中间开始
        crt_anim.lineClosing = false;  // 扫描线是展开状态
        crt_anim.lineWidth = 0;       // 扫描线宽度从0开始
    }
    // 关屏动画
    // 上下往中间收拢
    else
    {
        crt_anim.closing = true;
        crt_anim.doingLine = false;
        crt_anim.height = 0;
        crt_anim.lineClosing = true; // 扫描线是收缩状态
        crt_anim.lineWidth = FRAME_WIDTH;
    }

    crt_anim.active = true;
}

// 执行开屏和关屏动画
// 这个动画效果模拟了老式 CRT 显示器的开屏和关屏效果：
// 1. 开屏时：从中间向上下展开，然后有一个扫描线效果
// 2. 关屏时：先出现扫描线，然后从上下向中间收缩
// 动画通过控制显示缓冲区（oledBuffer）来实现，通过精确控制每个像素的状态来创建平滑的过渡效果。
// 这种动画不仅美观，而且能够给用户一个明确的视觉反馈，表明设备正在进入或退出睡眠状态。
/*
开屏动画：先线条动画，然后高度动画
1. 从屏幕中间开始（height = FRAME_HEIGHT / 2）
2. 直接开始扫描线效果（doingLine = true）
3. 扫描线从中间向两边展开（lineWidth 从0开始增加）
4. 当扫描线达到屏幕宽度时lineWidth=FRAME_WIDTH，动画切换成高度动画, doingLine = false
5. 高度动画从屏幕中间开始（height = FRAME_HEIGHT / 2）

这样的实现更符合 CRT 显示器的开屏效果：从屏幕中间出现一条扫描线，然后向两边展开。

关屏动画：先高度动画，然后线条动画
1. 从屏幕上下两端开始收缩（height 从0开始增加）
2. 当收缩到屏幕中间时（height = FRAME_HEIGHT / 2），切换到扫描线效果
3. 扫描线从两边向中间收缩（lineWidth 从屏幕宽度开始减少）
4. 当扫描线完全收缩时（lineWidth = 0），动画结束
这样的实现模拟了 CRT 显示器的关屏效果：先是从上下向中间收缩，然后出现一条扫描线，最后扫描线从两边向中间收缩消失。
*/
static void crt_animation()
{
    byte height = crt_anim.height;
    byte lineWidth = crt_anim.lineWidth;

    // 高度动画
    if (!crt_anim.doingLine)
    {
        // 根据是开屏还是关屏决定高度变化方向
        if (crt_anim.closing)
        {
            // 关屏时, 高度增加
            height += 3;
        }
        else
        {
            // 开屏时, 高度减少
            height -= 3;
        }

        // 当高度达到屏幕一半时，切换到扫描线效果
        // 只有关屏时，高度动画才会达到屏幕一半, 只有关屏时才会执行下边的代码
        if (height >= FRAME_HEIGHT / 2)
        {
            // 如果关屏, 则高度设置为一半, 并切换到线条动画
            if (crt_anim.closing)
            {
                height = FRAME_HEIGHT / 2;
                crt_anim.doingLine = true; // 切换到线条动画, 扫描线从两边向中间收缩
            }
            // 如果开屏, 则高度设置为0, 并关闭动画
            // 会执行下面的代码? 不会
            else
            {
                height = 0;
                crt_anim.active = false;
            }
            // crt_anim.closing = !crt_anim.closing;
        }
    }
    // 线条动画 crt_anim.doingLine = true
    else
    {
        // 根据是开屏还是关屏决定线条宽度变化
        if (crt_anim.lineClosing)
        {
            lineWidth -= 6; // 关屏时线条收缩
        }
        else
        {
            lineWidth += 10; // 开屏时线条扩张
        }

        // 只有开屏时，线条宽度才会达到最大, 才会执行下边的代码
        // 如果线条宽度达到最大
        if (lineWidth >= FRAME_WIDTH)
        {
            // 如果关屏状态线宽度变为0
            // 这个代码不会执行
            if (crt_anim.lineClosing)
            {
                lineWidth = 0;
            }
            // 开屏状态线宽度变为屏幕宽度
            else
            {
                lineWidth = FRAME_WIDTH;
            }

            // 如果扫描线是展开状态，则切换到高度动画
            if (!crt_anim.lineClosing)
            {
                crt_anim.doingLine = false;
            }

            // 如果扫描线是收缩状态，并且是关屏动画，则关闭动画
            // 这个代码不会执行, lineClosing = true表示是关屏动画
            if (crt_anim.lineClosing && crt_anim.closing)
            {
                crt_anim.active = false;
            }

            // 状态转换? 有什么用
            crt_anim.lineClosing = !crt_anim.lineClosing;
        }
    }

    // Full rows 
    // 根据高度计算需要填充的行数
    byte rows = height / 8;
    LOOP(rows, i)
    {
        // 清空上下两端的行
        memset(&oledBuffer[i * FRAME_WIDTH], 0, FRAME_WIDTH);
        memset(&oledBuffer[FRAME_BUFFER_SIZE - FRAME_WIDTH - (i * FRAME_WIDTH)], 0, FRAME_WIDTH);
    }

    // 绘制部分行和边缘线
    byte prows = height % 8;

    if (prows)
    { // Partial rows & edge line   
        // 计算起始和结束索引
        uint idxStart = rows * FRAME_WIDTH;
        uint idxEnd = ((FRAME_BUFFER_SIZE - 1) - idxStart);
        // 计算渐变边缘
        byte a = (255 << prows);
        byte b = (255 >> prows);
        byte c = (1 << prows);
        byte d = (128 >> prows);
        // 绘制渐变边缘
        LOOP(FRAME_WIDTH, i)
        {
            oledBuffer[idxStart] = (oledBuffer[idxStart] & a) | c;
            idxStart++;

            oledBuffer[idxEnd] = (oledBuffer[idxEnd] & b) | d;
            idxEnd--;
        }
    }
    else if (height)
    { // Edge line
        uint pos = ((byte)(FRAME_WIDTH - lineWidth) / 2) + ((byte)(FRAME_HEIGHT - height) / 8) * FRAME_WIDTH;
        memset(&oledBuffer[pos], 0x01, lineWidth);

        if (height != FRAME_HEIGHT / 2)
        {
            pos = (height / 8) * FRAME_WIDTH;
            LOOPR(FRAME_WIDTH, x)
            oledBuffer[pos + x] |= 0x01;
        }
    }

    crt_anim.height = height;
    crt_anim.lineWidth = lineWidth;

    //	if(crt_anim.doingLine && crt_anim.closing)
    //		draw_bitmap_s2(&crtdotImage);
}
