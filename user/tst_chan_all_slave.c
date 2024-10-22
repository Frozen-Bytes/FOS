// Test the Sleep and wakeup on a channel
// Slave program: sleep, increment test after wakeup
#include <inc/lib.h>

void
_main(void)
{
	int envID = sys_getenvid();

	//Sleep on the channel
	sys_utilities("__Sleep__", 0);

	//indicates wakenup
	inctst();

	return;
}
