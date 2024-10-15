#include <inc/lib.h>

void high_complexity_function();

void _main(void)
{
	high_complexity_function() ;
	return;
}

void high_complexity_function()
{
	uint32 end1 = RAND(0, 5000);
	uint32 end2 = RAND(0, 5000);
	int x = 10;
	for(int i = 0; i <= end1; i++)
	{
		for(int i = 0; i <= end2; i++)
		{
			{
				 x++;
			}
		}
	}
}


