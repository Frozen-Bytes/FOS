#include <inc/stdio.h>
#include <inc/error.h>
#include <inc/lib.h>

//static char buf[BUFLEN];

void readline(const char *prompt, char* buf)
{
	int i, c, echoing;

	if (prompt != NULL)
		cprintf("%s", prompt);

	i = 0;
	echoing = iscons(0);
	while (1) {
		c = getchar();
		if (c < 0) {
			if (c != -E_EOF)
				cprintf("read error: %e\n", c);
			break;
		} else if (c >= ' ' && i < BUFLEN-1) {
			if (echoing)
				cputchar(c);
			buf[i++] = c;
		} else if (c == '\b' && i > 0) {
			if (echoing)
				cputchar(c);

			i--;
		} else if (c == '\n' || c == '\r') {
			if (echoing)
				cputchar(c);

			buf[i] = 0;
			break;
		}
	}
}

void atomic_readline(const char *prompt, char* buf)
{
	sys_lock_cons();
	{
		int i, c, echoing;

		if (prompt != NULL)
			cprintf("%s", prompt);

		i = 0;
		echoing = iscons(0);
		while (1) {
			c = getchar();
			if (c < 0) {
				if (c != -E_EOF)
					cprintf("read error: %e\n", c);
				break;
			} else if (c >= ' ' && i < BUFLEN-1) {
				if (echoing)
					cputchar(c);
				buf[i++] = c;
			} else if (c == '\b' && i > 0) {
				if (echoing)
					cputchar(c);
				i--;
			} else if (c == '\n' || c == '\r') {
				if (echoing)
					cputchar(c);
				buf[i] = 0;
				break;
			}
		}
	}
	sys_unlock_cons();
}
