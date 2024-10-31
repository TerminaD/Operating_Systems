/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
		pushl $(num);							\
		jmp _alltraps
		
/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
		pushl $0;							\
		pushl $(num);							\
		jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 * TODO
 */

TRAPHANDLER_NOEC(divide_hdlr, T_DIVIDE)
TRAPHANDLER_NOEC(debug_hdlr, T_DEBUG)
TRAPHANDLER(nmi_hdlr, T_NMI)
TRAPHANDLER_NOEC(brkpt_hdlr, T_BRKPT)
TRAPHANDLER_NOEC(oflow_hdlr, T_OFLOW)
TRAPHANDLER_NOEC(bound_hdlr, T_BOUND)
TRAPHANDLER_NOEC(illop_hdlr, T_ILLOP)
TRAPHANDLER_NOEC(device_hdlr, T_DEVICE)
TRAPHANDLER(dblflt_hdlr, T_DBLFLT)
TRAPHANDLER(tss_hdlr, T_TSS)
TRAPHANDLER(segnp_hdlr, T_SEGNP)
TRAPHANDLER(stack_hdlr, T_STACK)
TRAPHANDLER(gpflt_hdlr, T_GPFLT)
TRAPHANDLER(pgflt_hdlr, T_PGFLT)
TRAPHANDLER_NOEC(fperr_hdlr, T_FPERR)
TRAPHANDLER(align_hdlr, T_ALIGN)
TRAPHANDLER_NOEC(mchk_hdlr, T_MCHK)
TRAPHANDLER_NOEC(simderr_hdlr, T_SIMDERR)

TRAPHANDLER_NOEC(syscall_hdlr, T_SYSCALL)

TRAPHANDLER_NOEC(irq0_hdlr, 32)
TRAPHANDLER_NOEC(irq1_hdlr, 33)
TRAPHANDLER_NOEC(irq2_hdlr, 34)
TRAPHANDLER_NOEC(irq3_hdlr, 35)
TRAPHANDLER_NOEC(irq4_hdlr, 36)
TRAPHANDLER_NOEC(irq5_hdlr, 37)
TRAPHANDLER_NOEC(irq6_hdlr, 38)
TRAPHANDLER_NOEC(irq7_hdlr, 39)
TRAPHANDLER_NOEC(irq8_hdlr, 40)
TRAPHANDLER_NOEC(irq9_hdlr, 41)
TRAPHANDLER_NOEC(irq10_hdlr, 42)
TRAPHANDLER_NOEC(irq11_hdlr, 43)
TRAPHANDLER_NOEC(irq12_hdlr, 44)
TRAPHANDLER_NOEC(irq13_hdlr, 45)
TRAPHANDLER_NOEC(irq14_hdlr, 46)
TRAPHANDLER_NOEC(irq15_hdlr, 47)

/*
 * Lab 3: Your code here for _alltraps
 * TODO
 */

_alltraps:
	/* push software-managed Trapframe items in reverse order */
	pushl %ds
	pushl %es
	pushal
	movw $(GD_KD),%ax
	movw %ax,%ds
	movw %ax,%es
	pushl %esp
	call trap