
#include <inc/lib.h>


int64 fibonacci(int n);

void
_main(void)
{
	int i1=0;
	char buff1[256];

	atomic_readline("Please enter Fibonacci index:", buff1);

	i1 = strtol(buff1, NULL, 10);

	int64 res = fibonacci(i1) ;

	atomic_cprintf("%@Fibonacci #%d = %lld\n",i1, res);

	return;
}


int64 fibonacci(int n)
{
	if (n <= 1)
		return 1 ;
	return fibonacci(n-1) + fibonacci(n-2) ;
}

