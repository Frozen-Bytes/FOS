/* See COPYRIGHT for copyright information. */

/* The Run Time Clock and other NVRAM access functions that go with it. */
/* The run time clock is hard-wired to IRQ8. */

#include "kclock.h"

#include <inc/x86.h>
#include <inc/stdio.h>
#include <inc/isareg.h>
#include <inc/timerreg.h>
#include <inc/lib.h>
#include <inc/assert.h>

#include <kern/cpu/picirq.h>
#include <kern/cpu/cpu.h>
#include <kern/cpu/sched.h>
#include <kern/trap/trap.h>


unsigned
mc146818_read(unsigned reg)
{
	outb(IO_RTC, reg);
	return inb(IO_RTC+1);
}

void
mc146818_write(unsigned reg, unsigned datum)
{
	outb(IO_RTC, reg);
	outb(IO_RTC+1, datum);
}



/* Every time the mode/command register is written to, all internal logic in the selected
 * PIT channel is reset, and the output immediately goes to its initial state
 * (which depends on the mode).
 */

void kclock_init()
{
	ticks = 0;
	irq_install_handler(0, &clock_interrupt_handler);
}
void
kclock_start(uint8 quantum_in_ms)
{
	//uint16 cnt0 = kclock_read_cnt0() ;

	/* initialize 8253 clock to interrupt N times/sec, N = 1 sec / CLOCK_INTERVAL */
	outb(TIMER_MODE, TIMER_SEL0 | TIMER_RATEGEN | TIMER_16BIT);

	//2017
//	outb(TIMER_CNTR0, TIMER_DIV((1000/CLOCK_INTERVAL_IN_MS)) % 256);
//	outb(TIMER_CNTR0, TIMER_DIV((1000/CLOCK_INTERVAL_IN_MS)) / 256);
	if (IS_VALID_QUANTUM(quantum_in_ms))
	{
		outb(TIMER_MODE, TIMER_SEL0 | TIMER_RATEGEN | TIMER_16BIT);
		kclock_write_cnt0_LSB_first(TIMER_DIV((1000/quantum_in_ms))) ;
	}
	else
	{
		panic("attempt to set the CPU quantum by too large value. Quantum should be between 1 ms and %d ms", QUANTUM_LIMIT - 1);
	}

	//===============

//	uint16 cnt0_before = kclock_read_cnt0() ;

//	int h, c = 0 ;
//	for (h = 0 ; h < 30000 ; h++)
//	{
//		c++ ;
//	}
//	cprintf("c = %d\n", c) ;

//	uint16 cnt0_after = kclock_read_cnt0() ;

	//cprintf("	Setup IRQ0 (timer interrupts) via 8259A\n");

	//irq_setmask_8259A(irq_mask_8259A & ~(1<<0));
	irq_clear_mask(0);

	//cprintf("	unmasked timer interrupt\n");

	//cprintf("Timer STARTED: Counter0 Before Lag = %d, After lag = %d\n", cnt0_before, cnt0_after );

}

void
kclock_stop(void)
{
//	int h, c = 0 ;
//			for (h = 0 ; h < 30000 ; h++)
//			{
//				c++ ;
//			}
//			cprintf("c = %d\n", c) ;

	//Read Status Register
	//outb(TIMER_MODE, 0xe0);
	//uint8 status = inb(TIMER_CNTR0) ;

	//outb(TIMER_MODE, TIMER_SEL0 | TIMER_16BIT);
	//outb(TIMER_CNTR0, 0x00) ;
	//outb(TIMER_CNTR0, 0x00) ;


	outb(TIMER_MODE, TIMER_SEL0 | TIMER_RATEGEN | TIMER_16BIT);

//	uint16 cnt0 = kclock_read_cnt0() ;
//	cprintf("Timer STOPPED: Counter0 = %d\n", cnt0 );
//
//	uint16 cnt0_before = kclock_read_cnt0();
//
//	int h, c = 0 ;
//	for (h = 0 ; h < 3000000 ; h++)
//	{
//		c++ ;
//	}
//	cprintf("STOP LAG = %d\n", c) ;
//
//	uint16 cnt0_after = kclock_read_cnt0() ;
//	cprintf("Timer STOPPED: Counter0 Before Lag = %d, After lag = %d\n", cnt0_before, cnt0_after );

//	for (int i = 0 ; i <20; i++)
//	{
//		uint16 cnt0 = kclock_read_cnt0();
//		cprintf("STOP AFTER: cnt0 = %d\n",cnt0);
//	}

	/*Mask the IRQ0 (Timer Interrupt)*/
	//irq_setmask_8259A(0xFFFF);
	irq_set_mask(0);

//	uint16 cnt0 = kclock_read_cnt0() ;
//	cprintf("Timer STOPPED: Counter0 Value = %x\n", cnt0 );
	//cprintf("Timer STOPPED: Status Value = %x\n", status);


}

void
kclock_resume(void)
{
	/*2024: changed to latch
	 * the current count is copied into an internal "latch register" which can then be read via the data port corresponding to the selected channel (I/O ports 0x40 to 0x42). The value kept in the latch register remains the same until it has been fully read, or until a new mode/command register is written.
	 * The main benefit of the latch command is that it allows both bytes of the current count to be read without inconsistencies. For example, if you didn't use the latch command, then the current count may decrease from 0x0200 to 0x01FF after you've read the low byte but before you've read the high byte, so that your software thinks the counter was 0x0100 instead of 0x0200 (or 0x01FF).
	 */
	//uint16 cnt0 = kclock_read_cnt0() ;
	uint16 cnt0 = kclock_read_cnt0_latch() ;
	//cprintf("CLOCK RESUMED: Counter0 Value = %d\n", cnt0 );
	//2017: if the remaining time is small, then increase it a bit to avoid invoking the CLOCK INT
	//		before returning back to the environment (this cause INT inside INT!!!) el7 :)
	if (cnt0 < 20)
	{
		cnt0 = 20;
	}

	if (cnt0 % 2 == 1)
		cnt0++;

	outb(TIMER_MODE, TIMER_SEL0 | TIMER_RATEGEN | TIMER_16BIT);
	kclock_write_cnt0_LSB_first(cnt0) ;

	//Busy-wait until the new cnt value is loaded from CR (Count Register) to CE (Count Element)
	//see PIT reference
//	uint8 status = 0xFF ;
//	int c = 0;
//	while ((status & 0x40) != 0)
//	{
//		//Read Status Register
//		outb(TIMER_MODE, 0xe2);
//		status = inb(TIMER_CNTR0) ;
////		cprintf("status checking %d", c++);
////		cprintf(", status register = %x\n", status);
//	}

//	uint16 cnt0_before = kclock_read_cnt0() ;

//	int h, c = 0 ;
//	for (h = 0 ; h < 300000 ; h++)
//	{
//		c++ ;
//	}
//	cprintf("c = %d\n", c) ;

//	uint16 cnt0_after = kclock_read_cnt0() ;
//	cprintf("Timer RESUMED: Counter0 Before Lag = %d, After lag = %d\n", cnt0_before, cnt0_after );


	//cprintf("	Setup IRQ0: timer interrupts via 8259A\n");
	//irq_setmask_8259A(irq_mask_8259A & ~(1<<0));
	irq_clear_mask(0);
	//cprintf("	unmasked timer interrupt\n");
}



//==============

void kclock_start_counter(uint8 cnt0)
{
	outb(TIMER_MODE, TIMER_SEL0 | TIMER_RATEGEN | TIMER_16BIT);
	kclock_write_cnt0_LSB_first(cnt0) ;
	//irq_setmask_8259A(irq_mask_8259A & ~(1<<0));
	irq_clear_mask(0);
}

//2018
//Reset the CNT0 to the given quantum value without affecting the interrupt status
void kclock_set_quantum(uint8 quantum_in_ms)
{
	if (IS_VALID_QUANTUM(quantum_in_ms))
	{
		/*2023*/
//		int cnt = TIMER_DIV((1000/quantum_in_ms));
//		if (cnt%2 == 1)
//			cnt++;
		int cnt = NUM_CLKS_PER_QUANTUM(quantum_in_ms);


		//cprintf("QUANTUM is set to %d ms (%d)\n", quantum_in_ms, TIMER_DIV((1000/quantum_in_ms)));
		outb(TIMER_MODE, TIMER_SEL0 | TIMER_RATEGEN | TIMER_16BIT);
		kclock_write_cnt0_LSB_first(cnt) ;
		kclock_stop();
		//uint16 cnt0 = kclock_read_cnt0_latch() ; //read after write to ensure it's set to the desired value
		//cprintf("\nkclock_set_quantum: clock after stop = %d\n",cnt0);
	}
	else
	{
		panic("attempt to set the CPU quantum by too large value. Quantum should be between 1 ms and %d ms", QUANTUM_LIMIT - 1);
	}
}
//==============


//2017
void
kclock_write_cnt0_LSB_first(uint16 val)
{
	/*You must prevent other code from setting the PIT channel's reload value or reading
	 * its current count once you've sent the lowest 8 bits. Disabling interrupts works
	 * for single CPU computers
	 * */
	pushcli();	//disable interrupt
	outb(TIMER_CNTR0, (uint8)(val & 0x00FF));
	outb(TIMER_CNTR0, (uint8)((val>>8) & 0x00FF));
	popcli();	//enable interrupt

}
//==============


uint16
kclock_read_cnt0(void)
{
	pushcli();	//disable interrupt
	uint8 cnt0_lo =  inb(TIMER_CNTR0);
	uint8 cnt0_hi =  inb(TIMER_CNTR0);
	uint16 cnt0 = (cnt0_hi << 8) | cnt0_lo ;
	popcli();	//enable interrupt
	return cnt0 ;
}


/* the current count is copied into an internal "latch register" which can then be read via
 * the data port corresponding to the selected channel (I/O ports 0x40 to 0x42).
 * The value kept in the latch register remains the same until it has been fully read,
 * or until a new mode/command register is written.
 * The main benefit of the latch command is that it allows both bytes of the current count
 * to be read without inconsistencies. For example, if you didn't use the latch command,
 * then the current count may decrease from 0x0200 to 0x01FF after you've read the low byte
 * but before you've read the high byte, so that your software thinks the counter was 0x0100
 * instead of 0x0200 (or 0x01FF).
 * REF: OSDev Wiki
 */
uint16
kclock_read_cnt0_latch(void)
{
	uint8 old_mode = inb(TIMER_MODE) ;
	outb(TIMER_MODE, TIMER_SEL0 | TIMER_LATCH);

	uint8 cnt0_lo =  inb(TIMER_CNTR0);
	uint8 cnt0_hi =  inb(TIMER_CNTR0);
	uint16 cnt0 = (cnt0_hi << 8) | cnt0_lo ;
	outb(TIMER_MODE, old_mode);

	return cnt0 ;
}

// __inline struct uint64
// get_virtual_time()
// {

// /*
 // * 	uint16 cx;
	// uint16 dx;
		// __asm __volatile("int %3\n"
		// : "=c" (cx), "=d" (dx)
		// : "a" (0),
		  // "i" (26)
		  // //: "ax", "cx", "dx"
		// );
		// uint32 result = (cx<<16) | dx;
// */
	// //	uint32 oldVal = rcr4();
	// //	lcr4(0);


	// struct uint64 result;

	// __asm __volatile("rdtsc\n"
	// : "=a" (result.low), "=d" (result.hi)
	// );

	// /*
	// uint32 low;
	// uint32 hi;
	// uint32 cx,eaxp,ebxp,ecxp,edxp ;
	// //; read APERF
	// cpuid(6, &eaxp, &ebxp, &ecxp, &edxp);

// //	__asm __volatile("movl 6, %eax\ncpuid\n"//bt %%ecx, 0\nmov %%ecx,%0"
// //		//	:	"=c" (cx)
// //			//: "=a" (low), "=d" (hi)
// //		);

// //	__asm __volatile("rdmsr"
// //					: "=a" (low), "=d" (hi)
// //			);


	// //char* ptr=(char*)&ecxp;
	// //ptr[3]=0;
	// //cprintf("as str = %s\n", ptr);
	// cprintf("ax = %x, bx = %x, cx = %x, dx = %x\n", eaxp,ebxp,ecxp,edxp);
	// */

	// return result;
// }

