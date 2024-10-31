#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>

void sched_halt(void);

// Choose a user environment to run and run it.
void
sched_yield(void)
{
	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env this CPU was
	// last running.  Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment. Make sure curenv is not null before
	// dereferencing it.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING). If there are
	// no runnable environments, simply drop through to the code
	// below to halt the cpu.

	// LAB 4: Your code here.
	// TODO

	// Old version:
	// ----------------------------------------------------------
	// size_t envx, cur_envx;

	// if (curenv == NULL) {
	// 	for (size_t envx = 0; envx < NENV; envx++)
	// 		if (envs[envx].env_status == ENV_RUNNABLE)
	// 			env_run(&envs[envx]);
	// } else {
	// 	size_t cur_envx = ENVX(curenv->env_id);
	// 	size_t envx;

	// 	if (cur_envx == NENV-1)		envx = 0;
	// 	else						envx = cur_envx+1;

	// 	for (; envx != cur_envx;) {
	// 		if (envs[envx].env_status == ENV_RUNNABLE)
	// 			env_run(&(envs[envx]));

	// 		if (envx == NENV-1)		envx = 0;
	// 		else					envx++;
	// 	}

	// 	if (envs[cur_envx].env_status == ENV_RUNNING)
	// 		env_run(&(envs[cur_envx]));
	// }

	// // sched_halt never returns
	// sched_halt();
	// ----------------------------------------------------------

	// New version: priority
	// ----------------------------------------------------------
	size_t envx, cur_envx, best_choice = 0;
	uint8_t min_priority = __UINT8_MAX__;

	if (curenv == NULL) {	// Starting from kernel
		for (envx = 0; envx < NENV; envx++) {
			if (envs[envx].env_status == ENV_RUNNABLE && envs[envx].env_priority < min_priority) {
				min_priority = envs[envx].env_priority;
				best_choice = envx;
			}
		}

		if (min_priority == __UINT8_MAX__)
			sched_halt();

		env_run(&envs[best_choice]);

	} else {	// Starting from another env
		cur_envx = ENVX(curenv->env_id);
		envx = (cur_envx+1) % NENV;

		while (envx != cur_envx) {
			if (envs[envx].env_status == ENV_RUNNABLE && envs[envx].env_priority < min_priority) {
				min_priority = envs[envx].env_priority;
				best_choice = envx;
			}
			envx = (envx+1) % NENV;
		}

		if (min_priority == __UINT8_MAX__) {	// No runnable envs
			if (envs[cur_envx].env_status == ENV_RUNNING)
	 			env_run(&(envs[cur_envx]));
			else
				sched_halt();
		}

		env_run(&envs[best_choice]);
	}

	sched_halt();
}

// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
//
void
sched_halt(void)
{
	int i;

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_DYING))
			break;
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");
		while (1)
			monitor(NULL);
	}

	// Mark that no environment is running on this CPU
	curenv = NULL;
	lcr3(PADDR(kern_pgdir));

	// Mark that this CPU is in the HALT state, so that when
	// timer interupts come in, we know we should re-acquire the
	// big kernel lock
	xchg(&thiscpu->cpu_status, CPU_HALTED);

	// Release the big kernel lock as if we were "leaving" the kernel
	unlock_kernel();

	// Reset stack pointer, enable interrupts and then halt.
	asm volatile (
		"movl $0, %%ebp\n"
		"movl %0, %%esp\n"
		"pushl $0\n"
		"pushl $0\n"
		// Uncomment the following line after completing exercise 13
		"sti\n"
		"1:\n"
		"hlt\n"
		"jmp 1b\n"
	: : "a" (thiscpu->cpu_ts.ts_esp0));
}

