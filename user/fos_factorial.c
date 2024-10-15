
#include <inc/lib.h>


int64 factorial(int n);

void
_main(void)
{
	int i1=0;
	char buff1[256];
	atomic_readline("Please enter a number:", buff1);
	i1 = strtol(buff1, NULL, 10);

	int64 res = factorial(i1) ;

	atomic_cprintf("Factorial %d = %lld\n",i1, res);
	return;
}


int64 factorial(int n)
{
	if (n <= 1)
		return 1 ;
	return n * factorial(n-1) ;
}

