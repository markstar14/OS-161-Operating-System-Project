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
#include <addrspace.h>
#include <vm.h>
#include <spl.h>
#include <mips/tlb.h>
/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */


#define DUMBVM_STACKPAGES    12



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
*/


struct addrspace *
as_create(void)
{
	struct addrspace *as;

	as = kmalloc(sizeof(struct addrspace));
	if (as == NULL) {
		return NULL;
	}

	/*
	 * Initialize as needed.
	 */

//	as->page_table = kmalloc(sizeof(struct page));
//	as->stack = NULL;
//	as->heap = NULL;

//	set the code region
	as->code_vbase = (vaddr_t)0;
	as->code_page_number = 0;
	as->page_table = NULL;

	//static region set up
	as->static_vbase = (vaddr_t)0;
	as->static_page_number = 0;
	as->static_region_start = NULL;

//	set the heap region
	as->heap_start = (vaddr_t)0;
	as->heap_end = (vaddr_t)0;
//	as->dynamic_end = (paddr_t)0;
	as->heap_region_start = NULL;


//	set the stack region
	as->stack_bottom = (vaddr_t)0;
	as->stack_region_start = NULL;
//	as->stack_end = (vaddr_t)0;
//	as->stack_paddress = (paddr_t)0;


	return as;
}

/*
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
/*      if (as_prepare_load(new)) {
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

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *newas;

	newas = as_create();
	if (newas==NULL) {
		return ENOMEM;
	}
	/*
	 * Write this.
	 */

	struct page *pagepointer = old->page_table;
	struct page *newpages;

	bool firstpage = true;

	if(old->page_table != NULL){
		while(pagepointer != NULL){
			if(firstpage){
				newas->page_table = (struct page*)kmalloc(sizeof(struct page));
				firstpage = false;
				newpages = newas->page_table;
			}
			else{
				newpages->next = (struct page*)kmalloc(sizeof(struct page));
				newpages = newpages->next;
			}
			newpages->virtualAddress = pagepointer->virtualAddress;
			newpages->write_flag = pagepointer->write_flag;
			newpages->read_flag = pagepointer->read_flag;
			newpages->exec_flag = pagepointer->exec_flag;
			newpages->physicalAddress = page_alloc(1);
			if(newpages->physicalAddress == 0){
				as_destroy(newas);
				return ENOMEM;
			}
			memmove((void *)PADDR_TO_KVADDR(newpages->physicalAddress), (const void *)PADDR_TO_KVADDR(pagepointer->physicalAddress), PAGE_SIZE);
//			memmove((void *)PADDR_TO_KVADDR(new->as_stackpbase), (const void *)PADDR_TO_KVADDR(old->as_stackpbase),DUMBVM_STACKPAGES*PAGE_SIZE);
			newpages->next = NULL;
//			if(firstpage){
//				newas->page_table = newpages;
//				firstpage = false;
//			}
//			newpages = newpages->next;
			pagepointer = pagepointer->next;
		}
		
//		newas->page_table = temppage;
		newas->code_vbase = old->page_table->virtualAddress;
		newas->code_page_number = old->code_page_number;
	}


	if(old->static_region_start != NULL){
		pagepointer = old->static_region_start;
		firstpage = true;
               	while(pagepointer != NULL){
			if(firstpage){
                                newas->static_region_start = (struct page*)kmalloc(sizeof(struct page));
                                firstpage = false;
                                newpages = newas->static_region_start;
                        }
                        else{
                                newpages->next = (struct page*)kmalloc(sizeof(struct page));
                                newpages = newpages->next;
                        }

//                       	newpages = (struct page*)kmalloc(sizeof(struct page));
                       	newpages->virtualAddress = pagepointer->virtualAddress;
                       	newpages->write_flag = pagepointer->write_flag;
                       	newpages->read_flag = pagepointer->read_flag;
                       	newpages->exec_flag = pagepointer->exec_flag;
                       	newpages->physicalAddress = page_alloc(1);
                       	if(newpages->physicalAddress == 0){        
                        	as_destroy(newas);
                                return ENOMEM;
                        }
			memmove((void *)PADDR_TO_KVADDR(newpages->physicalAddress), (const void *)PADDR_TO_KVADDR(pagepointer->physicalAddress), PAGE_SIZE);

                       	newpages->next = NULL;
//                       	if(firstpage){
//                               	newas->static_region_start = newpages;
//                               	firstpage = false;
//                       	}
//                       	newpages = newpages->next;
			pagepointer = pagepointer->next;
               	}
//             	newas->page_table = temppage;
               	newas->static_vbase = old->static_region_start->virtualAddress;
               	newas->static_page_number = old->static_page_number;
       	}
		

	if(old->heap_region_start != NULL){
                pagepointer = old->heap_region_start;
                firstpage = true;
                while(pagepointer != NULL){
  
			if(firstpage){
                                newas->heap_region_start = (struct page*)kmalloc(sizeof(struct page));
                                firstpage = false;
                                newpages = newas->heap_region_start;
                        }
                        else{
                                newpages->next = (struct page*)kmalloc(sizeof(struct page));
                                newpages = newpages->next;
                        }

//                      newpages = (struct page*)kmalloc(sizeof(struct page));
                        newpages->virtualAddress = pagepointer->virtualAddress;
                        newpages->write_flag = pagepointer->write_flag;
                        newpages->read_flag = pagepointer->read_flag;
                        newpages->exec_flag = pagepointer->exec_flag;
                        newpages->physicalAddress = page_alloc(1);
                        if(newpages->physicalAddress == 0){
                                as_destroy(newas);
                                return ENOMEM;
                        }
			memmove((void *)PADDR_TO_KVADDR(newpages->physicalAddress), (const void *)PADDR_TO_KVADDR(pagepointer->physicalAddress), PAGE_SIZE);

                        newpages->next = NULL;
//                        if(firstpage){
//                                newas->heap_region_start = newpages;
//                                firstpage = false;
//                        }
//                        newpages = newpages->next;
			pagepointer = pagepointer->next;
                }
//              newas->page_table = temppage;
                newas->heap_start = old->heap_start;
                newas->heap_end = old->heap_end;
        }

	if(old->stack_region_start != NULL){
                pagepointer = old->stack_region_start;
                firstpage = true;
                while(pagepointer != NULL){
			if(firstpage){
                                newas->stack_region_start = (struct page*)kmalloc(sizeof(struct page));
                                firstpage = false;
                                newpages = newas->stack_region_start;
                        }
                        else{
                                newpages->next = (struct page*)kmalloc(sizeof(struct page));
                                newpages = newpages->next;
                        }

//                        newpages = (struct page*)kmalloc(sizeof(struct page));
                        newpages->virtualAddress = pagepointer->virtualAddress;
                        newpages->write_flag = pagepointer->write_flag;
                        newpages->read_flag = pagepointer->read_flag;
                        newpages->exec_flag = pagepointer->exec_flag;
                        newpages->physicalAddress = page_alloc(1);
                        if(newpages->physicalAddress == 0){
                                as_destroy(newas);
                                return ENOMEM;
                        }
			memmove((void *)PADDR_TO_KVADDR(newpages->physicalAddress), (const void *)PADDR_TO_KVADDR(pagepointer->physicalAddress), PAGE_SIZE);

                        newpages->next = NULL;
//                        if(firstpage){
//                                newas->stack_region_start = newpages;
//                                firstpage = false;
//                        }
//                        newpages = newpages->next;
			pagepointer = pagepointer->next;
                }
//              newas->page_table = temppage;
                newas->stack_bottom = old->stack_bottom;
        }


	(void)old;
	
	*ret = newas;
	return 0;
}

/*
void
as_destroy(struct addrspace *as)
{
        kfree(as);
}
*/

void
as_destroy(struct addrspace *as)
{
	/*
	 * Clean up as needed.
	 */
	if(as != NULL){
		struct page* temp1 = as->page_table;
		struct page* temp2;

		//free the code region
		while((temp1 != NULL)&&(temp1 != (struct page *)0xdeadbeef)){
			temp2 = temp1;
			temp1 = temp2->next;
			freeoneaddress(temp2->physicalAddress);
			kfree(temp2);
		}

		//free the static region
		temp1 = as->static_region_start;
		while((temp1 != NULL)&&(temp1 != (struct page *)0xdeadbeef)){
			 temp2 = temp1;
                        temp1 = temp2->next;
                        freeoneaddress(temp2->physicalAddress);
                        kfree(temp2);
		}

		//free the heap region
		temp1 = as->heap_region_start;
                while((temp1 != NULL)&&(temp1 != (struct page *)0xdeadbeef)){
                         temp2 = temp1;
                        temp1 = temp2->next;
                        freeoneaddress(temp2->physicalAddress);
                        kfree(temp2);
                }

		//free the stack region. This code duplication pains me.
		temp1 = as->stack_region_start;
                while((temp1 != NULL)&&(temp1 != (struct page *)0xdeadbeef)){
                         temp2 = temp1;
                        temp1 = temp2->next;
                        freeoneaddress(temp2->physicalAddress);
                        kfree(temp2);
                }


		
	}	

	kfree(as);
}

/*void
as_activate(struct addrspace *as)
{
        int i, spl;

        (void)as;
*/
        /* Disable interrupts on this CPU while frobbing the TLB. */
/*      spl = splhigh();

        for (i=0; i<NUM_TLB; i++) {
                tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
        }

        splx(spl);
}
*/


void
as_activate(struct addrspace *as)
{
	/*
	 * Write this.
	 */

        int i, spl;

        (void)as;

        /* Disable interrupts on this CPU while frobbing the TLB. */
        spl = splhigh();

        for (i=0; i<NUM_TLB; i++) {
                tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
        }

        splx(spl);


}

/*
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
                 int readable, int writeable, int executable)
{
        size_t npages; 
*/
        /* Align the region. First, the base... */
/*      sz += vaddr & ~(vaddr_t)PAGE_FRAME;
        vaddr &= PAGE_FRAME;

*/      /* ...and now the length. */
/*      sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

        npages = sz / PAGE_SIZE;
*/
        /* We don't use these - all pages are read-write */
/*      (void)readable;
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
/*      kprintf("dumbvm: Warning: too many regions\n");
        return EUNIMP;
}
*/
/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */

int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
		 int readable, int writeable, int executable)
{
	/*
	 * Write this.
	 */

	size_t npages; 

        /* Align the region. First, the base... */
        sz += vaddr & ~(vaddr_t)PAGE_FRAME;
        vaddr &= PAGE_FRAME;

        /* ...and now the length. */
        sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

        npages = sz / PAGE_SIZE;


	//the first call will set up the code region
	if(as->code_vbase == 0){	
		as->code_page_number = npages;
		as->code_vbase = vaddr;
		as->page_table = (struct page *)kmalloc(sizeof(struct page));
		as->page_table->next = NULL;
		as->page_table->write_flag = writeable;
		as->page_table->read_flag = readable;
		as->page_table->exec_flag = executable;
		as->page_table->virtualAddress = vaddr;
		as->page_table->physicalAddress = 0;
		return 0;
	}


	//the second call will set up the static region and the heap bacause why not?
	if(as->static_vbase == 0){
		as->static_vbase = vaddr;
		as->static_page_number = npages;
		as->static_region_start = (struct page*)kmalloc(sizeof(struct page));
		as->static_region_start->next = NULL;
		as->static_region_start->write_flag = writeable;
		as->static_region_start->read_flag = readable;
		as->static_region_start->exec_flag = executable;
		as->static_region_start->virtualAddress = vaddr;
		as->static_region_start->physicalAddress = 0;	

		//I'll move this down to as_prepare_load since it looks nicer down there
/*		as->heap_start = vaddr + sz;
		as->heap_region_start = (struct page *)kmalloc(sizeof(struct page));
		as->heap_region_start->next = NULL;
		as->heap_region_start->virtualAddress = as->heap_start;
		as->heap_region_start->physicalAddress = 0;
		as->heap_region_start->write_flag = 1;
		as->heap_region_start->read_flag = 1;
		as->heap_region_start->exec_flag = 0;		
*/
		return 0;
	}
	kprintf("dumbvm: Warning: too many regions\n");
        return EUNIMP;


/*	(void)as;
	(void)vaddr;
	(void)sz;
	(void)readable;
	(void)writeable;
	(void)executable;
	return EUNIMP;
*/

}



/*
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
*/


int
as_prepare_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */
	
	size_t i;
	vaddr_t vaddr = as->page_table->virtualAddress;
	struct page* page_pointer = as->page_table;


	//loops through the code page table and sets up the region with the flags and addresses
	for(i = 0; i < as->code_page_number; i++){
		if(i == 0){
			page_pointer->physicalAddress = page_alloc(1); 
			if(page_pointer->physicalAddress == 0){
				return ENOMEM;
			}
		}
		else{
			page_pointer->next = (struct page*)kmalloc(sizeof(struct page));
			page_pointer = page_pointer->next;
			page_pointer->virtualAddress = vaddr;
			page_pointer->write_flag = as->page_table->write_flag;
			page_pointer->read_flag = as->page_table->read_flag;
			page_pointer->exec_flag = as->page_table->exec_flag;
			page_pointer->physicalAddress = page_alloc(1);
			if(page_pointer->physicalAddress == 0){
				return ENOMEM;
			}		
			page_pointer->next = NULL;
		}
		vaddr = vaddr + PAGE_SIZE;
	}

	//now do the same with the static region. code duplication sucks, but I'll leave it for now
	page_pointer = as->static_region_start;
	vaddr = as->static_region_start->virtualAddress;

	for(i = 0; i < as->static_page_number; i++){
                if(i == 0){
                        page_pointer->physicalAddress = page_alloc(1);
                        if(page_pointer->physicalAddress == 0){
                                return ENOMEM;
                        }
                }
                else{
                        page_pointer->next = (struct page*)kmalloc(sizeof(struct page));
			page_pointer = page_pointer->next; 
                        page_pointer->virtualAddress = vaddr;
                        page_pointer->write_flag = as->page_table->write_flag;
                        page_pointer->read_flag = as->page_table->read_flag;
                        page_pointer->exec_flag = as->page_table->exec_flag;
                        page_pointer->physicalAddress = page_alloc(1);
                        if(page_pointer->physicalAddress == 0){
                                return ENOMEM;
                        }
                        page_pointer->next = NULL;
                }
                vaddr = vaddr + PAGE_SIZE;
        }


	//I'll just set up the heap real quick since I have the vaddr already, there isn't much anyway
//	page_pointer = as->heap_region_start;
	as->heap_region_start = (struct page*)kmalloc(sizeof(struct page));
	as->heap_region_start->virtualAddress = vaddr;
	as->heap_region_start->physicalAddress = page_alloc(1);
	if(as->heap_region_start->physicalAddress == 0){
		return ENOMEM;
	}
	as->heap_region_start->write_flag = 1;
	as->heap_region_start->read_flag = 1;
	as->heap_region_start->exec_flag = 0;
	as->heap_region_start->next = NULL;

	//heap start and heap end
	as->heap_start = vaddr;
	as->heap_end = vaddr;



	//now for the stack......yay...., I joke, but it's probably going to be similar to the code and static regions I'll just use the same number of stack pages as DUMBVM
	vaddr = USERSTACK - (DUMBVM_STACKPAGES * PAGE_SIZE);
//	page_pointer = as->stack_region_start;
	for(i = 0; i < DUMBVM_STACKPAGES; i++){
		
		if(i == 0){
			as->stack_region_start = (struct page *)kmalloc(sizeof(struct page));
			page_pointer = as->stack_region_start;

		}
		else {
			page_pointer->next = (struct page *)kmalloc(sizeof(struct page));
			page_pointer = page_pointer->next;
		}

		page_pointer->virtualAddress = vaddr;
		page_pointer->physicalAddress = page_alloc(1);
		if(page_pointer->physicalAddress == 0){
			return ENOMEM;
		}
		page_pointer->next =  NULL;
		page_pointer->write_flag = 1;
		page_pointer->read_flag = 1;
		page_pointer->exec_flag = 0;
		
		vaddr = vaddr + PAGE_SIZE;
//		page_pointer = page_pointer->next;
	}
	as->stack_bottom = as->stack_region_start->virtualAddress;

	return 0;
}

/*
int
as_complete_load(struct addrspace *as)
{
        (void)as;
        return 0;
}

*/

int
as_complete_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */
//	do I really need to do anythig here? the blog says to change the flags back, but I have doubts about that. We'll see I guess

	(void)as;
	return 0;
}

/*
int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
        KASSERT(as->as_stackpbase != 0);

        *stackptr = USERSTACK;
        return 0;
}
*/

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	/*
	 * Write this.
	 */

	(void)as;

	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;
//	stackptr should be the top of the stack, so I guess I don't have to do anything here either	
	return 0;
}

