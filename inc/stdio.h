#ifndef FOS_INC_STDIO_H
#define FOS_INC_STDIO_H

#include <inc/stdarg.h>

#ifndef NULL
#define NULL	((void *) 0)
#endif /* !NULL */

#define BUFLEN 1024
#define NAMELEN 64

/*2023*/ //Moved here instead of lib/printf.c
unsigned char printProgName ;

// lib/stdio.c
void	cputchar(int c);
int	getchar(void);
int	iscons(int fd);

// lib/printfmt.c
void	printfmt(void (*putch)(int, void*), void *putdat, const char *fmt, ...);
void	vprintfmt(void (*putch)(int, void*), void *putdat, const char *fmt, va_list);

// lib/printf.c
int	cprintf(const char *fmt, ...);
int cprintf_colored(int color, const char *fmt, ...);
int	atomic_cprintf(const char *fmt, ...);
int	vcprintf(const char *fmt, va_list);
int	vcprintf_colored(const char *fmt, va_list);

// defines for cprintf_colored

// forground colors (text colors)
enum
{
    TEXT_black = 0x0,
    TEXT_blue,
    TEXT_green,
    TEXT_cyan,
    TEXT_red,
    TEXT_magenta,
    TEXT_brown,
    TEXT_light_grey,
    TEXT_dark_grey,
    TEXT_light_blue,
    TEXT_light_green,
    TEXT_light_cyan,
    TEXT_light_red,
    TEXT_light_magenta,
    TEXT_yellow,
    TEXT_white
};

// lib/sprintf.c
int	snprintf(char *str, int size, const char *fmt, ...);
int	vsnprintf(char *str, int size, const char *fmt, va_list);

// lib/fprintf.c
int	printf(const char *fmt, ...);
int	fprintf(int fd, const char *fmt, ...);
int	vfprintf(int fd, const char *fmt, va_list);

// lib/readline.c
void readline(const char *prompt, char*);

#endif /* !FOS_INC_STDIO_H */
