// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	// TODO

	uintptr_t int_addr = (uintptr_t)addr;

	// if (!((err & FEC_WR) && 
	//       (uvpd[PDX(int_addr)] & PTE_P) && 
	// 	  (uvpt[PGNUM(int_addr)] & PTE_P) && 
	// 	  (uvpt[PGNUM(int_addr)] & PTE_COW))) 
	// 	panic("In lib/fork.c:pgfault: faulting access not write, or page not present, or page not COW");

	// cprintf("-----------------------\n");
	// cprintf("err: ");
	// if (err & FEC_PR) cprintf("Protection violation ");
	// if (err & FEC_WR) cprintf("Write ");
	// if (err & FEC_U) cprintf("In user mode ");
	// cprintf("\n");
	// cprintf("faulting addr: %p\n", addr);
	// cprintf("current envid: 0x%x\n", thisenv->env_id);
	// cprintf("PTE content: 0x%x\n", uvpt[PGNUM(int_addr)]);

	if (!(uvpd[PDX(int_addr)] & PTE_P))
		panic("Page table not present");
	if (!(uvpt[PGNUM(int_addr)] & PTE_P)) {
		cprintf("[fork.c/pgfault] int_addr: %p\n", addr);
		cprintf("[fork.c/pgfault] pte: 0x%x\n", uvpt[PGNUM(int_addr)]);
		panic("Page not present");
	}
	if (!(uvpt[PGNUM(int_addr)] & PTE_COW))
		panic("Page not COW");
	if (!(err & FEC_WR))
		panic("Faulting access not write");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	// TODO

	envid_t envid = sys_getenvid();

	if ((r = sys_page_alloc(envid, PFTEMP, PTE_P|PTE_U|PTE_W)))
		panic("In sys_page_alloc in lib/fork.c:pgfault: %e", r);
	// cprintf("Allocated temp page\n");

	if (memcpy(PFTEMP, ROUNDDOWN(addr, PGSIZE), PGSIZE) == NULL)
		panic("In lib/fork.c:pgfault: memcpy fails");
	// cprintf("Copied page to temp page\n");

	if ((r = sys_page_map(envid, PFTEMP, envid, ROUNDDOWN(addr, PGSIZE), PTE_P|PTE_U|PTE_W)))
		panic("In sys_page_map in lib/fork.c:pgfault: %e", r);
	// cprintf("Mapped temp page to faulting address\n");

	if ((r = sys_page_unmap(envid, PFTEMP)))
        panic("In sys_page_unmap in lib/fork.c:pgfault: %e", r);
	// cprintf("Unmapped temp page\n");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;
	envid_t prnt_envid = sys_getenvid();

	// LAB 4: Your code here.
	// TODO

	// The page is guarenteed to be writable or COW

	void *va = (void *)(pn * PGSIZE);
	pte_t pte = uvpt[PGNUM(va)];

	if (pte & PTE_COW) {
		if ((r = sys_page_map(prnt_envid, va, envid, va, PTE_P|PTE_U|PTE_COW)))
			return r;
		if ((r = sys_page_map(prnt_envid, va, prnt_envid, va, PTE_P|PTE_U|PTE_COW)))
			return r;

	} else if (pte & PTE_SHARE) {
		if ((r = sys_page_map(prnt_envid, va, envid, va, PTE_SYSCALL & pte)))
			return r;

	} else if (pte & PTE_W) {
		if ((r = sys_page_map(prnt_envid, va, envid, va, PTE_P|PTE_U|PTE_COW)))
			return r;
		if ((r = sys_page_map(prnt_envid, va, prnt_envid, va, PTE_P|PTE_U|PTE_COW)))
			return r;
			
	} else {
		if ((r = sys_page_map(prnt_envid, va, envid, va, PTE_P|PTE_U)))
			return r;
	}

	// if (pte & PTE_SHARE) {	// Shared region, copy mapping
	// 	if ((r = sys_page_map(prnt_envid, va, envid, va, PTE_SYSCALL & pte)))	// Not sure about this
	// 		return r;

	// } else if ((pte & PTE_W) || (pte & PTE_COW)) {	// Writable or copy-on-write
	// 	if ((r = sys_page_map(prnt_envid, va, envid, va, PTE_P|PTE_U|PTE_COW)))
	// 		return r;
	// 	if ((r = sys_page_map(prnt_envid, va, prnt_envid, va, PTE_P|PTE_U|PTE_COW)))
	// 		return r;

	// } else {	// Read-only pages, suffice to copy mapping
	// 	if ((r = sys_page_map(prnt_envid, va, envid, va, PTE_P|PTE_U)))
	// 		return r;
	// }
	
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	// TODO

	envid_t chld_envid;
	pte_t pte;
	int r;

	// Install pgfault() as C-level page fault handler
	set_pgfault_handler(pgfault);

	// Create children
	if ((chld_envid = sys_exofork()) > 0) {	// In parent
		envid_t prnt_envid = sys_getenvid();

		// Everything below exception stack
		for (uintptr_t va = 0; va < USTACKTOP; va += PGSIZE) {

			if (uvpd[PDX(va)] & PTE_P)	// Page table is present
				pte = uvpt[PGNUM(va)];
			else
				continue;

			if (pte & PTE_P)		// Page is present
				duppage(chld_envid, PGNUM(va));
		}

		// Duplicate exception stack
		// cprintf("fork is copying exception stack address: %p\n", UXSTACKTOP-PGSIZE);
		if ((r = sys_page_alloc(chld_envid, (void *)(UXSTACKTOP-PGSIZE), PTE_P|PTE_U|PTE_W))) return r;

		// Set child's page fault entry point (upcall)
		if ((r = sys_env_set_pgfault_upcall(chld_envid, thisenv->env_pgfault_upcall))) return r;

		// Mark child as runnable
		if ((r = sys_env_set_status(chld_envid, ENV_RUNNABLE))) return r;

		return chld_envid;

	} else if (chld_envid == 0) {			// In child
		// Fix "thisenv" in the child process
		thisenv = &envs[ENVX(sys_getenvid())];

		return 0;

	} else							// Error
		return chld_envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
