/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <spinlock.h>
#include <thread.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>

/*
 * Dumb MIPS-only "VM system" that is intended to only be just barely
 * enough to struggle off the ground. You should replace all of this
 * code while doing the VM assignment. In fact, starting in that
 * assignment, this file is not included in your kernel!
 */

/* under dumbvm, always have 48k of user stack */
#define DUMBVM_STACKPAGES    12

/*
 * Wrap rma_stealmem in a spinlock.
 */
static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;

struct coremap_page* coremap;
int total_page_num;
paddr_t firstaddr, lastaddr, freeaddr;
bool booted = false;
int TLBindex = 0;

void
vm_bootstrap(void)
{
//	paddr_t firstaddr, lastaddr;
	ram_getsize(&firstaddr, &lastaddr);
//	total_page_num = ROUNDDOWN(lastaddr, PAGE_SIZE) / PAGE_SIZE;
	total_page_num = (lastaddr - firstaddr) / PAGE_SIZE;
	coremap = (struct coremap_page*)PADDR_TO_KVADDR(firstaddr);
	freeaddr = firstaddr + total_page_num * sizeof(struct coremap_page);

	//I need to make this ROUNDDOWN or something
	//int fixedPageMaxIndex = (freeaddr - firstaddr) / PAGE_SIZE;

	//ROUNDUP

	for(int i = 0; i < total_page_num; i++){

		coremap[i].virtualAddress = PADDR_TO_KVADDR(PAGE_SIZE * i + firstaddr);
		coremap[i].physicalAddress = PAGE_SIZE * i + firstaddr;
		if(coremap[i].physicalAddress < freeaddr){//i < fixedPageMaxIndex){
			coremap[i].state = FIXED;//0;//"fixed"; strings didn't work, so I'll use 0
		}
		else{
			coremap[i].state = FREE;//1;//"free"; I guess I'll make free = 1
		}
	}
	//flag for vm booted? will it be a number or something? a bool?
	booted = true;
	int temp = free_core_page_number();
	kprintf("%d", temp);
}

int free_core_page_number(){
	spinlock_acquire(&stealmem_lock);
	int freecount = 0;
	for(int i = 0; i < total_page_num; i++){
		if(coremap[i].state == FREE){
			freecount++;
		}
	}
	spinlock_release(&stealmem_lock);
	return freecount;
}

static
paddr_t
getppages(unsigned long npages)
{
	paddr_t addr;

	spinlock_acquire(&stealmem_lock);

	addr = ram_stealmem(npages);
	
	spinlock_release(&stealmem_lock);
	return addr;
}

paddr_t page_alloc(int npages){
	int index = 0;
        if(booted){

//		int temp = free_core_page_number();
//        kprintf("\n page_alloc %d\n", temp);

                spinlock_acquire(&stealmem_lock);
                int contig_free_pages = 0;
                int start_free_pages = 0;
                bool in_free_block = false;
                bool found_entire_block = false;


                for(index = 0; index < total_page_num; index++){
                        if(coremap[index].state == FREE){
                                contig_free_pages++;
                                if(in_free_block == false){
                                        start_free_pages = index;
                                        in_free_block = true;
                                }
                                if(contig_free_pages == npages){
                                        found_entire_block = true;
                                        break;
                                }
                        }
                        else{
                                in_free_block = false;
                                contig_free_pages = 0;
                        }
                }
		

		
                if(found_entire_block == false){
			kprintf("tough luck, no core map page left\n");
                        spinlock_release(&stealmem_lock);
                        return 0;
                }
                for(index = 0; index < npages; index++){
                        coremap[index + start_free_pages].state = DIRTY;
                        coremap[index + start_free_pages].first_block_index = start_free_pages;
                }
                spinlock_release(&stealmem_lock);
                return coremap[start_free_pages].physicalAddress;
        }
	else{
//              spinlock_acquire(&stealmem_lock);
                paddr_t pa;
                pa = getppages(npages);
                if (pa==0) {
                        return 0;
                }
//              spinlock_release(&stealmem_lock);
                return pa;
        }

}

/* Allocate/free some kernel-space virtual pages */
vaddr_t 
alloc_kpages(int npages)
{
	int index = 0;
	if(booted){

//		int temp = free_core_page_number();
//        kprintf("\n alloc_kpages %d\n", temp);

		spinlock_acquire(&stealmem_lock);
		int contig_free_pages = 0;
		int start_free_pages = 0;
		bool in_free_block = false;
		bool found_entire_block = false;


		for(index = 0; index < total_page_num; index++){
			if(coremap[index].state == FREE){
				contig_free_pages++;
				if(in_free_block == false){
					start_free_pages = index;
					in_free_block = true;
				}
				if(contig_free_pages == npages){
					found_entire_block = true;
					break;
				}
			}
			else{
				in_free_block = false;
				contig_free_pages = 0;
			}
		}
		if(found_entire_block == false){
			kprintf("sucks to be you, not enough coremap pages\n");
			spinlock_release(&stealmem_lock);
			return 0;
		}
		for(index = 0; index < npages; index++){
			coremap[index + start_free_pages].state = DIRTY;
			coremap[index + start_free_pages].first_block_index = start_free_pages;
		}
		spinlock_release(&stealmem_lock);
		return PADDR_TO_KVADDR(coremap[start_free_pages].physicalAddress);
	}
	else{
//		spinlock_acquire(&stealmem_lock);
		paddr_t pa;
		pa = getppages(npages);
		if (pa==0) {
			return 0;
		}
//		spinlock_release(&stealmem_lock);
		return PADDR_TO_KVADDR(pa);
	}
}

void 
free_kpages(vaddr_t addr)
{
	/* nothing - leak the memory. */

//	(void)addr;
	int index;
	int addr_start_index;
	spinlock_acquire(&stealmem_lock);

	for(index = 0; index < total_page_num; index++){
		if(coremap[index].virtualAddress == addr){
		
			addr_start_index = coremap[index].first_block_index;

			while(coremap[index].first_block_index == addr_start_index){
				coremap[index].state = FREE;
				index++;
			}

			break;
		}
	}

	spinlock_release(&stealmem_lock);
	
}

//helper function for as_destroy
void freeoneaddress(paddr_t pa){
	spinlock_acquire(&stealmem_lock);
	for (int i = 0; i<total_page_num; i++){
        	if(coremap[i].physicalAddress == pa){
                	coremap[i].state = FREE;
                        break;
                }
        }
	spinlock_release(&stealmem_lock);
}

void
vm_tlbshootdown_all(void)
{
	panic("dumbvm tried to do tlb shootdown?!\n");
}

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("dumbvm tried to do tlb shootdown?!\n");
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	vaddr_t stackbase, stacktop;//vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
	paddr_t paddr;
	int i;
	uint32_t ehi, elo;
	struct addrspace *as;
	int spl;
//	struct page* entry;

	faultaddress &= PAGE_FRAME;

	DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);

	switch (faulttype) {
	    case VM_FAULT_READONLY:
		/* We always create pages read-write, so we can't get this */
		panic("dumbvm: got VM_FAULT_READONLY\n");
	    case VM_FAULT_READ:
	    case VM_FAULT_WRITE:
		break;
	    default:
		return EINVAL;
	}

	as = curthread->t_addrspace;
	if (as == NULL) {
		/*
		 * No address space set up. This is probably a kernel
		 * fault early in boot. Return EFAULT so as to panic
		 * instead of getting into an infinite faulting loop.
		 */
		return EFAULT;
	}

	/* Assert that the address space has been set up properly. */
/*	KASSERT(as->as_vbase1 != 0);
	KASSERT(as->as_pbase1 != 0);
	KASSERT(as->as_npages1 != 0);
	KASSERT(as->as_vbase2 != 0);
	KASSERT(as->as_pbase2 != 0);
	KASSERT(as->as_npages2 != 0);
	KASSERT(as->as_stackpbase != 0);
	KASSERT((as->as_vbase1 & PAGE_FRAME) == as->as_vbase1);
	KASSERT((as->as_pbase1 & PAGE_FRAME) == as->as_pbase1);
	KASSERT((as->as_vbase2 & PAGE_FRAME) == as->as_vbase2);
	KASSERT((as->as_pbase2 & PAGE_FRAME) == as->as_pbase2);
	KASSERT((as->as_stackpbase & PAGE_FRAME) == as->as_stackpbase);

	vbase1 = as->as_vbase1;
	vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
	vbase2 = as->as_vbase2;
	vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;
	stackbase = USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE;
	stacktop = USERSTACK;
*/
	KASSERT(as->page_table != NULL);
	KASSERT(as->static_region_start != NULL);
	KASSERT(as->heap_region_start != NULL);
	KASSERT(as->stack_region_start != NULL);

	KASSERT((as->code_vbase & PAGE_FRAME) == (as->code_vbase));
	KASSERT(as->code_page_number != 0);
	
	KASSERT((as->static_vbase & PAGE_FRAME) == (as->static_vbase));
	KASSERT(as->static_page_number != 0);

	

	stackbase = USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE;
        stacktop = USERSTACK;



/*	if (faultaddress >= vbase1 && faultaddress < vtop1) {
		paddr = (faultaddress - vbase1) + as->as_pbase1;
	}
	else if (faultaddress >= vbase2 && faultaddress < vtop2) {
		paddr = (faultaddress - vbase2) + as->as_pbase2;
	}
	else if (faultaddress >= stackbase && faultaddress < stacktop) {
		paddr = (faultaddress - stackbase) + as->as_stackpbase;
	}
	else {
		return EFAULT;
	}
*/


	//check the stack for the fault address first since that is easier to check for in an if statement

	//start going through the page table to find the fault address
	struct page *pagepointer;
	paddr = (paddr_t)0;

		

	if((faultaddress >= as->stack_bottom) && (faultaddress < USERSTACK)){
		pagepointer = as->stack_region_start;
	}
	else if(faultaddress >= as->heap_start){
		pagepointer = as->heap_region_start;
	}
	else if(faultaddress >= as->static_vbase){
		pagepointer = as->static_region_start;
	}
	else if(faultaddress >= as->code_vbase){
		pagepointer = as->page_table;
	}
	else{
	//it's not in any of the regions so return EFAULT or something
		return EFAULT;
	}

	while(pagepointer != NULL){
                if((faultaddress >= pagepointer->virtualAddress) && (faultaddress < (pagepointer->virtualAddress + PAGE_SIZE))){
                        //paddr = (faultaddress - stackbase) + as->as_stackpbase;
                        paddr = (faultaddress - pagepointer->virtualAddress) + pagepointer->physicalAddress;
                        break;
                }
                pagepointer = pagepointer->next;
        }

	if(paddr == 0){
		return EFAULT;
	}


	/* make sure it's page-aligned */
	KASSERT((paddr & PAGE_FRAME) == paddr);

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

//	kprintf("%d", TLBindex);	
	for (i=0; i<NUM_TLB; i++) {
		tlb_read(&ehi, &elo, i);
		if (elo & TLBLO_VALID) {
			continue;
		}
//		kprintf("%p and %p\n",  (void *)paddr, (void *)faultaddress);
		ehi = faultaddress;
		elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
		DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
		tlb_write(ehi, elo, i);
		splx(spl);
		return 0;
	}
//	kprintf("M14");	
//	i = rand() % 63;
	ehi = faultaddress;
	elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
	tlb_write(ehi, elo, TLBindex);
	splx(spl);
	TLBindex++;
	if(TLBindex == 63){
		TLBindex = 0;
	}
	return 0;

/*	kprintf("dumbvm: Ran out of TLB entries - cannot handle page fault\n");
	splx(spl);
	return EFAULT;
*/
}

/*struct addrspace *
as_create(void)
{
	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	if (as==NULL) {
		return NULL;
	}

	as->as_vbase1 = 0;
	as->as_pbase1 = 0;
	as->as_npages1 = 0;
	as->as_vbase2 = 0;
	as->as_pbase2 = 0;
	as->as_npages2 = 0;
	as->as_stackpbase = 0;

	return as;
}

void
as_destroy(struct addrspace *as)
{
	kfree(as);
}
*/

/*void
as_activate(struct addrspace *as)
{
	int i, spl;

	(void)as;
*/
	/* Disable interrupts on this CPU while frobbing the TLB. */
/*	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	splx(spl);
}

int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
		 int readable, int writeable, int executable)
{
	size_t npages; 
*/
	/* Align the region. First, the base... */
/*	sz += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

*/	/* ...and now the length. */
/*	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = sz / PAGE_SIZE;
*/
	/* We don't use these - all pages are read-write */
/*	(void)readable;
	(void)writeable;
	(void)executable;

	if (as->as_vbase1 == 0) {
		as->as_vbase1 = vaddr;
		as->as_npages1 = npages;
		return 0;
	}

	if (as->as_vbase2 == 0) {
		as->as_vbase2 = vaddr;
		as->as_npages2 = npages;
		return 0;
	}
*/
	/*
	 * Support for more than two regions is not available.
	 */
/*	kprintf("dumbvm: Warning: too many regions\n");
	return EUNIMP;
}

static
void
as_zero_region(paddr_t paddr, unsigned npages)
{
	bzero((void *)PADDR_TO_KVADDR(paddr), npages * PAGE_SIZE);
}

int
as_prepare_load(struct addrspace *as)
{
	KASSERT(as->as_pbase1 == 0);
	KASSERT(as->as_pbase2 == 0);
	KASSERT(as->as_stackpbase == 0);

	as->as_pbase1 = getppages(as->as_npages1);
	if (as->as_pbase1 == 0) {
		return ENOMEM;
	}

	as->as_pbase2 = getppages(as->as_npages2);
	if (as->as_pbase2 == 0) {
		return ENOMEM;
	}

	as->as_stackpbase = getppages(DUMBVM_STACKPAGES);
	if (as->as_stackpbase == 0) {
		return ENOMEM;
	}
	
	as_zero_region(as->as_pbase2, as->as_npages2);
	as_zero_region(as->as_stackpbase, DUMBVM_STACKPAGES);

	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	(void)as;
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	KASSERT(as->as_stackpbase != 0);

	*stackptr = USERSTACK;
	return 0;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *new;

	new = as_create();
	if (new==NULL) {
		return ENOMEM;
	}

	new->as_vbase1 = old->as_vbase1;
	new->as_npages1 = old->as_npages1;
	new->as_vbase2 = old->as_vbase2;
	new->as_npages2 = old->as_npages2;
*/
	/* (Mis)use as_prepare_load to allocate some physical memory. */
/*	if (as_prepare_load(new)) {
		as_destroy(new);
		return ENOMEM;
	}

	KASSERT(new->as_pbase1 != 0);
	KASSERT(new->as_pbase2 != 0);
	KASSERT(new->as_stackpbase != 0);

	memmove((void *)PADDR_TO_KVADDR(new->as_pbase1),
		(const void *)PADDR_TO_KVADDR(old->as_pbase1),
		old->as_npages1*PAGE_SIZE);

	memmove((void *)PADDR_TO_KVADDR(new->as_pbase2),
		(const void *)PADDR_TO_KVADDR(old->as_pbase2),
		old->as_npages2*PAGE_SIZE);

	memmove((void *)PADDR_TO_KVADDR(new->as_stackpbase),
		(const void *)PADDR_TO_KVADDR(old->as_stackpbase),
		DUMBVM_STACKPAGES*PAGE_SIZE);
	
	*ret = new;
	return 0;
}*/
