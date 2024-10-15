/* See COPYRIGHT for copyright information. */

#include <inc/assert.h>

#include "picirq.h"


// Current IRQ mask.
// Initial IRQ mask has interrupt 2 enabled (for slave 8259A).
uint16 irq_init_mask_8259A = 0xFFFF & ~(1<<IRQ_SLAVE);
static bool didinit;

/* Initialize the 8259A interrupt controllers. */
void
pic_init(void)
{
	didinit = 1;

	// mask all interrupts
	outb(PIC1_DATA, 0xFF);
	outb(PIC2_DATA, 0xFF);

	// Set up master (8259A-1)

	// ICW1:  0001g0hi
	//    g:  0 = edge triggering, 1 = level triggering
	//    h:  0 = cascaded PICs, 1 = master only
	//    i:  0 = no ICW4, 1 = ICW4 required
	outb(PIC1_CMD, 0x11);

	// ICW2:  Vector offset
	outb(PIC1_DATA, IRQ_OFFSET);

	// ICW3:  bit mask of IR lines connected to slave PICs (master PIC),
	//        3-bit No of IR line at which slave connects to master(slave PIC).
	outb(PIC1_DATA, 1<<IRQ_SLAVE);

	// ICW4:  000nbmap
	//    n:  1 = special fully nested mode
	//    b:  1 = buffered mode
	//    m:  0 = slave PIC, 1 = master PIC
	//	  (ignored when b is 0, as the master/slave role
	//	  can be hardwired).
	//    a:  1 = Automatic EOI mode
	//    p:  0 = MCS-80/85 mode, 1 = intel x86 mode
	outb(PIC1_DATA, 0x3);

	// Set up slave (8259A-2)
	outb(PIC2_CMD, 0x11);			// ICW1
	outb(PIC2_DATA, IRQ_OFFSET + 8);	// ICW2
	outb(PIC2_DATA, IRQ_SLAVE);		// ICW3
	// NB Automatic EOI mode doesn't tend to work on the slave.
	// Linux source code says it's "to be investigated".
	outb(PIC2_DATA, 0x01);			// ICW4

	// OCW3:  0ef01prs
	//   ef:  0x = NOP, 10 = clear specific mask, 11 = set specific mask
	//    p:  0 = no polling, 1 = polling mode
	//   rs:  0x = NOP, 10 = read IRR, 11 = read ISR
	outb(PIC1_CMD, 0x68);             /* clear specific mask */
	outb(PIC1_CMD, 0x0a);             /* read IRR by default */

	outb(PIC2_CMD, 0x68);               /* OCW3 */
	outb(PIC2_CMD, 0x0a);               /* OCW3 */

	if (irq_init_mask_8259A != 0xFFFF)
		irq_setmask_8259A(irq_init_mask_8259A);
}

void
irq_setmask_8259A(uint16 mask)
{
	/*2024: commented. It just reflect the initial mask value only
	 * We then use the new functions irq_set_mask() and irq_clear_mask()
	 * to manipulate a specific IRQ mask
	 */
	//irq_init_mask_8259A = mask;

	if (!didinit)
		return;

	outb(PIC1_DATA, (char)mask);
	outb(PIC2_DATA, (char)(mask >> 8));

	//cprintf("enabled interrupts:");
	//for (int i = 0; i < 16; i++)
	//if (~mask & (1<<i))
	//cprintf(" %d", i);
	//cprintf("\n");
}

/*Ref: OSDev Wiki*/
void irq_set_mask(uint8 IRQline)
{
	if (!didinit)
		return;

	uint16 port;
	uint8 value;

	if(IRQline < 8) {
		port = PIC1_DATA;
	} else {
		port = PIC2_DATA;
		IRQline -= 8;
	}
	value = inb(port) | (1 << IRQline);
	outb(port, value);
}

/*Ref: OSDev Wiki*/
void irq_clear_mask(uint8 IRQline)
{
	if (!didinit)
		return;

	uint16 port;
	uint8 value;

	if(IRQline < 8) {
		port = PIC1_DATA;
	} else {
		port = PIC2_DATA;
		IRQline -= 8;
	}
	value = inb(port) & ~(1 << IRQline);
	outb(port, value);
}


int irq_get_mask(uint8 IRQline)
{
	if (!didinit)
		return -1;

	uint16 port;
	uint8 value;

	if(IRQline < 8) {
		port = PIC1_DATA;
	} else {
		port = PIC2_DATA;
		IRQline -= 8;
	}
	value = inb(port) & (1 << IRQline);
	return value;
}

/*Perhaps the most common command issued to the PIC chips is the end of interrupt (EOI) command
 * (code 0x20). This is issued to the PIC chips at the end of an IRQ-based interrupt routine.
 * If the IRQ came from the Master PIC, it is sufficient to issue this command only to the
 * Master PIC; however if the IRQ came from the Slave PIC, it is necessary to issue the command
 * to both PIC chips.
 */
void pic_sendEOI(uint8 irq)
{
	if(irq >= 8)
		outb(PIC2_CMD,PIC_EOI);

	outb(PIC1_CMD,PIC_EOI);
}
