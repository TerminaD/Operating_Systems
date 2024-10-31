// Lab 4 challenge: priority scheduling
// Check if we've implemented priority scheduling correctly!
// umain forks off 5 children with priority 0 through 2. 
// They should run in this order!

#include <inc/lib.h>

void
umain(int argc, char **argv)
{	
	
	envid_t chld0, chld1, chld2;
	int r;
	
	if ((chld0 = fork()) > 0) {
		// "Dispatcher" process after creating child 0
		cprintf("Created new child: 0x%x\n", chld0);
		if ((r = sys_env_set_priority(chld0, 0))) panic("Failed to set child 0's priority: %e", r);

		if ((chld1 = fork()) > 0) {
			// "Dispatcher" process after creating child 1
			cprintf("Created new child: 0x%x\n", chld1);
			if ((r = sys_env_set_priority(chld1, 1))) panic("Failed to set child 1's priority: %e", r);

			if ((chld2 = fork()) > 0) {
				// "Dispatcher" process after creating child 2
				cprintf("Created new child: 0x%x\n", chld2);
				if ((r = sys_env_set_priority(chld2, 2))) panic("Failed to set child 2's priority: %e", r);
				sys_yield();
				cprintf("env 0x%x exiting!\n", thisenv->env_id);
				exit();

			} else if (chld2 == 0) {
				// Child 2
				sys_yield();
				cprintf("env 0x%x exiting!\n", thisenv->env_id);
				exit();

			} else panic("Failed to fork child 2!");

		} else if (chld1 == 0) {
			// Child 1
			sys_yield();
			cprintf("env 0x%x exiting!\n", thisenv->env_id);
			exit();

		} else panic("Failed to fork child 1!");

	} else if (chld0 == 0) {
		// Child 0
		sys_yield();
		cprintf("env 0x%x exiting!\n", thisenv->env_id);
		exit();

	} else panic("Failed to fork child 0!");
}

