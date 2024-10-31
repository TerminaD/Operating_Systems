
#include "fs.h"

#define MAXCACHE 16

static uint32_t ncache = 0;
static void *bcache[MAXCACHE];

// Return the virtual address of this disk block.
void*
diskaddr(uint32_t blockno)
{
	if (blockno == 0 || (super && blockno >= super->s_nblocks))
		panic("bad block number %08x in diskaddr", blockno);
	return (char*) (DISKMAP + blockno * BLKSIZE);
}

// Is this virtual address mapped?
bool
va_is_mapped(void *va)
{
	return (uvpd[PDX(va)] & PTE_P) && (uvpt[PGNUM(va)] & PTE_P);
}

// Is this virtual address dirty?
bool
va_is_dirty(void *va)
{
	return (uvpt[PGNUM(va)] & PTE_D) != 0;
}

// Lab 5 Challenge
// TODO
bool
va_is_accessed(void *va)
{
	return (uvpt[PGNUM(va)] & PTE_A);
}

// Fault any disk block that is read in to memory by
// loading it from disk.
static void
bc_pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t blockno = ((uint32_t)addr - DISKMAP) / BLKSIZE;
	int r;

	// cprintf("-----------------------------------------\n");
	// cprintf("[bc_pgfault] blockno: 0x%x\n", blockno);

	// Check that the fault was within the block cache region
	if (addr < (void*)DISKMAP || addr >= (void*)(DISKMAP + DISKSIZE))
		panic("page fault in FS: eip %08x, va %08x, err %04x",
		      utf->utf_eip, addr, utf->utf_err);

	// Sanity check the block number.
	if (super && blockno >= super->s_nblocks)
		panic("reading non-existent block %08x\n", blockno);

	// Allocate a page in the disk map region, read the contents
	// of the block from the disk into that page.
	// Hint: first round addr to page boundary. fs/ide.c has code to read
	// the disk.
	//
	// LAB 5: you code here:
	// TODO

	addr = ROUNDDOWN(addr, PGSIZE);
	if ((r = sys_page_alloc(0, addr, PTE_W | PTE_U | PTE_P)))
    	panic("in bc_pgfault, sys_page_alloc: %e", r);

	if ((r = ide_read(blockno * BLKSECTS, addr, BLKSECTS)))
    	panic("in bc_pgfault, ide_read: %e", r);

	// Clear the dirty bit for the disk block page since we just read the
	// block from disk
	if ((r = sys_page_map(0, addr, 0, addr, uvpt[PGNUM(addr)] & PTE_SYSCALL)) < 0)
		panic("in bc_pgfault, sys_page_map: %e", r);

	// Check that the block we read was allocated. (exercise for
	// the reader: why do we do this *after* reading the block
	// in?)
	if (bitmap && block_is_free(blockno))
		panic("reading free block %08x\n", blockno);

	// Lab 5 Challenge
	// TODO

	uint32_t i;
	void *curr_addr;

	// The superblock should never be cached
	if (addr < diskaddr(2))
		return;
	
	if (ncache < MAXCACHE) {	// Still room, add a block
		for (i = 0; i < MAXCACHE; i++) {	// Search through
			if (bcache[i] == NULL) {
				bcache[i] = addr;
				ncache++;
				return;
			}
		}
	} else {	// Evict all non-accessed pages, then add a block
		for (i = 0; i < MAXCACHE; i++) {
			curr_addr = bcache[i];
			if (va_is_mapped(curr_addr)) {
				if (!va_is_accessed(curr_addr)) {	// Not accessed, evict
					bcache[i] = NULL;
					ncache--;

					if (va_is_dirty(curr_addr))	// Flush dirty blocks back to disk
						flush_block(curr_addr);

					if ((r = sys_page_unmap(thisenv->env_id, curr_addr)) < 0)
						panic("in bc_pgfault, in sys_page_unmap: %e\n", r);
				}

			} else
				panic("in bc_pgfault: cached address not mapped");
		}

		// No block was evicted in the previous step, evict the first one
		if (ncache == MAXCACHE) {	
			curr_addr = bcache[0];

			if (va_is_dirty(curr_addr))
				flush_block(curr_addr);
			
			if ((r = sys_page_unmap(thisenv->env_id, curr_addr)) < 0)
				panic("in bc_pgfault, in sys_page_unmap: %e\n", r);

			bcache[0] = addr;

			return;
		}

		for (i = 0; i < MAXCACHE; i++) {	// Search through
			if (bcache[i] == NULL) {
				bcache[i] = addr;
				ncache++;
				return;
			}
		}
	}
}

// Flush the contents of the block containing VA out to disk if
// necessary, then clear the PTE_D bit using sys_page_map.
// If the block is not in the block cache or is not dirty, does
// nothing.
// Hint: Use va_is_mapped, va_is_dirty, and ide_write.
// Hint: Use the PTE_SYSCALL constant when calling sys_page_map.
// Hint: Don't forget to round addr down.
void
flush_block(void *addr)
{
	uint32_t blockno = ((uint32_t)addr - DISKMAP) / BLKSIZE;
	int r;

	if (addr < (void*)DISKMAP || addr >= (void*)(DISKMAP + DISKSIZE))
		panic("flush_block of bad va %08x", addr);

	// LAB 5: Your code here.
	// TODO

	void *rounded_addr = ROUNDDOWN(addr, PGSIZE);

	if (va_is_mapped(rounded_addr) && va_is_dirty(rounded_addr)) {
		// cprintf("[flush_block] need to flush\n");
		// Write back to disk
		if ((r = ide_write(blockno*BLKSECTS, rounded_addr, BLKSECTS)))
			panic("in flush_block, ide_write: %e", r);

		// Clear dirty bit
		if ((r = sys_page_map(0, rounded_addr, 0, rounded_addr, uvpt[PGNUM(addr)] & PTE_SYSCALL)) < 0)
			panic("in flush_block, sys_page_map: %e", r);
	}
}

// Test that the block cache works, by smashing the superblock and
// reading it back.
static void
check_bc(void)
{
	struct Super backup;

	// back up super block
	memmove(&backup, diskaddr(1), sizeof backup);

	// smash it
	strcpy(diskaddr(1), "OOPS!\n");
	flush_block(diskaddr(1));
	assert(va_is_mapped(diskaddr(1)));
	assert(!va_is_dirty(diskaddr(1)));

	// clear it out
	sys_page_unmap(0, diskaddr(1));
	assert(!va_is_mapped(diskaddr(1)));

	// read it back in
	assert(strcmp(diskaddr(1), "OOPS!\n") == 0);

	// fix it
	memmove(diskaddr(1), &backup, sizeof backup);
	flush_block(diskaddr(1));

	// Now repeat the same experiment, but pass an unaligned address to
	// flush_block.

	// back up super block
	memmove(&backup, diskaddr(1), sizeof backup);

	// smash it
	strcpy(diskaddr(1), "OOPS!\n");

	// Pass an unaligned address to flush_block.
	flush_block(diskaddr(1) + 20);
	assert(va_is_mapped(diskaddr(1)));

	// Skip the !va_is_dirty() check because it makes the bug somewhat
	// obscure and hence harder to debug.
	//assert(!va_is_dirty(diskaddr(1)));

	// clear it out
	sys_page_unmap(0, diskaddr(1));
	assert(!va_is_mapped(diskaddr(1)));

	// read it back in
	assert(strcmp(diskaddr(1), "OOPS!\n") == 0);

	// fix it
	memmove(diskaddr(1), &backup, sizeof backup);
	flush_block(diskaddr(1));

	cprintf("block cache is good\n");
}

void
bc_init(void)
{
	struct Super super;
	set_pgfault_handler(bc_pgfault);
	check_bc();

	// cache the super block by reading it once
	memmove(&super, diskaddr(1), sizeof super);

	// Lab 5 Challenge
	// TODO
	for (uint32_t i = 0; i < MAXCACHE; i++)
		bcache[i] = NULL;
}
