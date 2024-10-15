// Implementation of cprintf console output for user environments,
// based on printfmt() and the sys_cputs() system call.
//
// cprintf is a debugging statement, not a generic output statement.
// It is very important that it always go to the console, especially when
// debugging file descriptor code!

#include <inc/types.h>
#include <inc/stdio.h>
#include <inc/stdarg.h>
#include <inc/lib.h>

// Collect up to 256 characters into a buffer
// and perform ONE system call to print all of them,
// in order to make the lines output to the console atomic
// and prevent interrupts from causing context switches
// in the middle of a console output line and such.
struct printbuf {
	int idx; // current buffer index
	int cnt; // total bytes printed so far
	char buf[256];
};

static void putch(int ch, struct printbuf *b) {
	b->buf[b->idx++] = ch;
	if (b->idx == 256 - 1) {
		sys_cputs(b->buf, b->idx, printProgName);
		b->idx = 0;
	}
	b->cnt++;
}

int vcprintf(const char *fmt, va_list ap) {
	struct printbuf b;

	b.idx = 0;
	b.cnt = 0;
	vprintfmt((void*) putch, &b, fmt, ap);
	sys_cputs(b.buf, b.idx, printProgName);

	printProgName = 0;
	return b.cnt;
}

//%@: to print the program name and ID before the message
//%~: to print the message directly
int cprintf(const char *fmt, ...) {
	va_list ap;
	int cnt;
	printProgName = 1 ;
	va_start(ap, fmt);
	cnt = vcprintf(fmt, ap);
	va_end(ap);

	return cnt;
}

//%@: to print the program name and ID before the message
//%~: to print the message directly
int atomic_cprintf(const char *fmt, ...)
{
	int cnt;
	sys_lock_cons();
	{
		va_list ap;
		va_start(ap, fmt);
		cnt = vcprintf(fmt, ap);
		va_end(ap);
	}
	sys_unlock_cons();
	return cnt;
}
