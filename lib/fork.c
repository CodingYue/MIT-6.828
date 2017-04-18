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

	unsigned pn = (unsigned) addr / PGSIZE;
	pte_t pte = uvpt[pn];
	if ((utf->utf_err & FEC_WR) == 0 || (pte & PTE_COW) == 0) {
		panic("pgfault handler failed");
	}

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	addr = (void *) (pn * PGSIZE);
	if ((r = sys_page_map(0, addr, 0, PFTEMP, PTE_P|PTE_U)) < 0)
		panic("sys_page_map: %e", r);
	if ((r = sys_page_alloc(0, addr, PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_page_alloc: %e", r);
	memmove(addr, PFTEMP, PGSIZE);
	if ((r = sys_page_unmap(0, PFTEMP)) < 0)
		panic("sys_page_unmap: %e", r);
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
	int rtn;
	if ((uvpd[pn >> 10] & PTE_P) != 0) {
		pte_t pte = uvpt[pn];
		void *addr = (void *) (pn * PGSIZE);
		if ((pte & PTE_W) != 0 || (pte & PTE_COW) != 0) {
			if ((rtn = sys_page_map(0, addr, envid, addr, PTE_COW | PTE_U | PTE_P)) < 0) {
				return rtn;
			}
			if ((rtn = sys_page_map(0, addr, 0, addr, PTE_COW | PTE_U | PTE_P))) {
				return rtn;
			}
		}
	}
	
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
extern void _pgfault_upcall(void);
envid_t
fork(void)
{
	set_pgfault_handler(&pgfault);

	envid_t envid = sys_exofork();
	if (envid < 0) {
		panic("sys_exofork: %e", envid);
	}
	if (envid == 0) {
		// child process
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}
	uint8_t *addr;

	for (addr = (uint8_t*) UTEXT; addr < (uint8_t*) USTACKTOP; addr += PGSIZE) {
		duppage(envid, (unsigned) addr / PGSIZE);
	}

	sys_page_alloc(envid, (void*) (UXSTACKTOP-PGSIZE), PTE_U|PTE_P|PTE_W);
	sys_env_set_pgfault_upcall(envid, _pgfault_upcall);

	int rtn;
	if ((rtn = sys_env_set_status(envid, ENV_RUNNABLE)) < 0) {
		panic("sys_env_set_status: %e", rtn);
	}

	return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
