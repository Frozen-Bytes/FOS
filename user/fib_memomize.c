
#include <inc/lib.h>


int64 fibonacci(int n, int64 *memo);

void
_main(void)
{
	int index=0;
	char buff1[256];
	atomic_readline("Please enter Fibonacci index:", buff1);
	index = strtol(buff1, NULL, 10);

	int64 *memo = malloc((index+1) * sizeof(int64));
	for (int i = 0; i <= index; ++i)
	{
		memo[i] = 0;
	}
	int64 res = fibonacci(index, memo) ;

	free(memo);

	atomic_cprintf("Fibonacci #%d = %lld\n",index, res);
	//To indicate that it's completed successfully
		inctst();
	return;
}


int64 fibonacci(int n, int64 *memo)
{
	if (memo[n]!=0)	return memo[n];
	if (n <= 1)
		return memo[n] = 1 ;
	return (memo[n] = fibonacci(n-1, memo) + fibonacci(n-2, memo)) ;
}



