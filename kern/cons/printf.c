// Simple implementation of cprintf console output for the kernel,
// based on printfmt() and the kernel console's cputchar().

#include <inc/types.h>
#include <inc/stdio.h>
#include <inc/stdarg.h>
#include <kern/cpu/cpu.h>


static void
putch(int ch, int *cnt)
{
	cputchar(ch);
	(*cnt)++;
}

int
vcprintf(const char *fmt, va_list ap)
{
	int cnt = 0;

	vprintfmt((void*)putch, &cnt, fmt, ap);
	return cnt;
}

int
cprintf(const char *fmt, ...)
{
	//2024 - better to use locks instead (to support multiprocessors)
	int cnt;
	pushcli();	//disable interrupts
	{
		va_list ap;

		va_start(ap, fmt);
		cnt = vcprintf(fmt, ap);
		va_end(ap);
	}
	popcli();	//enable interrupts

	return cnt;
}

