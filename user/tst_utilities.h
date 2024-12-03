/*
 * tst_utilities.h
 *
 *  Created on: Oct 28, 2024
 *      Author: HP
 */

#ifndef USER_TST_UTILITIES_H_
#define USER_TST_UTILITIES_H_
#include <inc/types.h>
#include <inc/stdio.h>

int check_block(void* va, void* expectedVA, uint32 expectedSize, uint8 expectedFlag)
{
	//Check returned va
	if(va != expectedVA)
	{
		cprintf("wrong block address. Expected %x, Actual %x\n", expectedVA, va);
		return 0;
	}
	//Check header & footer
	uint32 header = *((uint32*)va-1);
	uint32 footer = *((uint32*)(va + expectedSize - 8));
	uint32 expectedData = expectedSize | expectedFlag ;
	if(header != expectedData || footer != expectedData)
	{
		cprintf("wrong header/footer data. Expected %d, Actual H:%d F:%d\n", expectedData, header, footer);
		return 0;
	}
	return 1;
}

#endif /* USER_TST_UTILITIES_H_ */
