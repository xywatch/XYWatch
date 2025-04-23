#ifndef _GUI_LOG_CONSOLE_H_
#define _GUI_LOG_CONSOLE_H_

typedef struct
{
	char str_buff[20];
} STRBUFF;

void console_loop_show(void);
void console_log(unsigned short time_delay, const char *fmt, ...);
bool up_line(void);
bool down_line(void);
void console_init(void);

#endif
