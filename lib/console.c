#include <inc/lib.h>


void
cputchar(int ch)
{
	char c = ch;

	// Unlike standard Unix's putchar,
	// the cputchar function _always_ outputs to the system console.
	//sys_cputs(&c, 1);

	sys_cputc(c);
}


int
getchar(void)
{
	int c =sys_cgetc();
	return c;
}

int iscons(int fdnum)
{
	// used by readline
	return 1;
}
