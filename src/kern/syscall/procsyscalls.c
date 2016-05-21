#include <types.h>
#include <lib.h>
#include <kern/proccalls.h>
#include <array.h>
#include <current.h>
#include <copyinout.h>
#include <vfs.h>
#include <vnode.h>
#include <filehandle.h>
#include <stat.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <mips/trapframe.h>
#include <uio.h>
#include <kern/iovec.h>
#include <kern/seek.h>
#include <limits.h>
#include <synch.h>
#include <filehandle.h>
#include <addrspace.h>
#include <vm.h>
#include <kern/syscall.h>
#include <syscall.h>
#include <kern/wait.h>
#include <spl.h>


pid_t sys_getpid(void){
	return curthread->pid;
}

int  sys_waitpid(pid_t pid, int *status, int options, int *retval){
	
	int result;


	if(pid > PID_LIMIT || pid < 1 || proc_tbl[pid] == NULL){

                return ESRCH;
        }       
        if((pid == proc_tbl[curthread->pid]->par_pid) || pid == curthread->pid || proc_tbl[pid]->par_pid != curthread->pid){
//        	if(pid == proc_tbl[curthread->pid]->par_pid){
//		kprintf("ECHILD\n");
//		}
	        return ECHILD;
        }

	if(options != 0){
		return EINVAL;
	}
	if(status == NULL || status > (int *)0x80000000){
		return EFAULT;
	}
/*	if(pid > PID_LIMIT || pid < 1 || proc_tbl[pid] == NULL){
		return ESRCH;
	}
	if((pid == proc_tbl[curthread->pid]->par_pid) || pid == curthread->pid || proc_tbl[pid]->par_pid != curthread->pid){
		return ECHILD;
	}
*/
	if(proc_tbl[pid]->exit == false){
		P(proc_tbl[pid]->sem);
	}

	result = copyout((const void *) &(proc_tbl[pid]->exit_code), (userptr_t) status, sizeof(int));
	if(result){
		return result;
	}

	*retval = pid;

//	sem_destroy(proc_tbl[pid]->sem);

	kfree(proc_tbl[pid]);
	proc_tbl[pid] = NULL;
	return 0;
	
}


int sys_execv(const char *program, char **args)
{		

	

        int result, arglen, index, bytenum;
        char **temparg, *progname;
	vaddr_t enter, stackptr;
//	char **copyarg;

//	size_t argsize;
	size_t realargsize;
	 
	int argc = 0;
//	if(!(result = strlen(program)))
//		return EISDIR;a

	struct  vnode *v;

	if (program == NULL || args == NULL){
		return EFAULT;
	}

//	copyarg = (char **) kmalloc(4096);

	progname = (char *)kmalloc(PATH_MAX);
	result = copyinstr((const_userptr_t)program, progname, PATH_MAX, &realargsize);
	if(result){
		kfree(progname);
		return result;
	}
	if(realargsize == 1){
		kfree(progname);
		return EINVAL;
	}

//	while(args[argc] != NULL){
		
//		result = copyin(args + index

//		argc++;
//	}

//	temparg = (char **)kmalloc(sizeof(char*) * (argc + 1));

	temparg = (char **) kmalloc(sizeof(char **));	
	//copy the argument array first in here
	result = copyin((const_userptr_t) args, temparg, sizeof(char **));
	if(result){
		kfree(progname);
		kfree(temparg);
		return EINVAL;
	}

//	for(index = 0; index <  argc; index++){
//		if((argsize = strlen(args[index])) > ARG_MAX){
//			return E2BIG;
//		}
//		temparg[index] = (char *)kmalloc(ARG_MAX);
//		result = copyinstr((const_userptr_t)args[index], temparg[index], ARG_MAX, NULL);
//		if(result){
//			if(result == ENAMETOOLONG)
//				return E2BIG;
//			return result;
//		}
//	}

//	now loop through the arg array and copy in each entry now that the whole array was copied in already.
	index = 0;
	while(args[index] != NULL){
		//allocate an entry into the copied in argument array and I don't know how big it is so it's of size PATH_MAX
		temparg[index] = (char *) kmalloc(sizeof(char) * PATH_MAX);

		//copy the argument into the new array
		result = copyinstr((const_userptr_t) args[index], temparg[index], PATH_MAX, &realargsize);

		if(result){
			for(int i = 0; i < index; i++){
				kfree(temparg[i]);
			}
			kfree(temparg);
			kfree(progname);
			return EFAULT;
		}

		index = index + 1;
//		temparg[index] = NULL;
	}

	temparg[index] = NULL;

	/* Open the file. */
        result = vfs_open(progname, O_RDONLY, 0, &v);
        kfree(progname);
	if (result) {
		for(int i = 0; i < index; i++){
                      kfree(temparg[i]);
                }
		kfree(temparg);
                return result;
        }

	if(curthread->t_addrspace != NULL){
		as_destroy(curthread->t_addrspace);
	}
//	curthread->t_addrspace = NULL;


        /* Create a new address space. */
        curthread->t_addrspace = as_create();
        if (curthread->t_addrspace==NULL) {
                vfs_close(v);
		for(int i = 0; i < index; i++){
			kfree(temparg[i]);
                }

		kfree(temparg);
                return ENOMEM;
        } 

        /* Activate it. */
        as_activate(curthread->t_addrspace);
        
	/* Load the executable. */
        result = load_elf(v, &enter);
        if (result) {
                /* thread_exit destroys curthread->t_addrspace */
                vfs_close(v);
		for(int i = 0; i < index; i++){
                                kfree(temparg[i]);
                        }

		kfree(temparg);
                return result;
        }

        /* Done with the file now. */
        vfs_close(v);

        /* Define the user stack in the address space */
        result = as_define_stack(curthread->t_addrspace, &stackptr);
        if (result) {
                /* thread_exit destroys curthread->t_addrspace */
		for(int i = 0; i < index; i++){
                                kfree(temparg[i]);
                        }
                kfree(temparg);

		return result;
        }



//	if(argc == 1){
//                temparg[1] = NULL;
//        }
//        else{
//                tempargs[numargs] = NULL;
//        }


        
//      int totallength;


//        temparg = (char **)kmalloc(sizeof(char*) * numargs);

	argc = index;

        for(index = 0, bytenum = 0; temparg[index] != NULL; index++){
                arglen = strlen(temparg[index])+1; //adding the 1 so I can have it be null terminated, as said on piazza
                bytenum = arglen; //arglen is the size of the arg, with no 0s
                bytenum = (4 - (bytenum % 4));//bytenum is the number of 0s needed to pad the argument on the stack
                char* onearg = kmalloc(sizeof(bytenum+arglen));//onearg should be bigger then the argument for the null terminating 0s
                onearg = kstrdup(temparg[index]);
                for(int i = 0; i < (bytenum + arglen); i++){
                        if(i < arglen){
                                onearg[i] = temparg[index][i];
                        }
                        else{
                                onearg[i] = '\0';
                        }
                } //onearg is now the entire argument, with padded 0s, that will be put on the stack
                result = strlen(onearg);
                stackptr -= (bytenum + arglen);
                result = copyout((const void *)onearg, (userptr_t)stackptr, (size_t)(bytenum+arglen)); //puts the current arg, with padding, onto the stack
                if(result){
			for(int i = 0; i < argc; i++){
                                kfree(temparg[i]);
                        }
			kfree(onearg);
			kfree(temparg);
			return result;
		}
//              totallength = totallength + bytenum + argnum;
//              temparg[index] = onearg;
		kfree(temparg[index]);
                temparg[index] = (char *)stackptr;
                kfree(onearg);
        }


	argc = index;

//	        result = sizeof(char*);
        for(; index >= 0; index--){//index < 0; index--){
                stackptr = stackptr - 4;
                if(temparg[index] != NULL){
                      result = copyout((const void *)(temparg+index), (userptr_t)stackptr, (sizeof(char *)));
//		      result = copyout((const void *)(temparg[index]), (userptr_t)stackptr, (sizeof(char*)));  
                      if(result) {
				for(int i = 0; i < argc; i++){
                        	        kfree(temparg[i]);
                	        }
				
				kfree(temparg);
                                return result;
                        }
                }
        }

//	for(int i = 0; i < argc; i++){
//                    kfree(temparg[i]);
//        }

	kfree(temparg);

	/* Warp to user mode. */
        enter_new_process(argc /*argc*/, (userptr_t)stackptr /*userspace addr of argv*/, stackptr, enter);

        /* enter_new_process does not return. */
        panic("enter_new_process returned\n");
        return EINVAL;

}

int sys_fork(struct trapframe *tf, int *retval){

	struct trapframe *child_tf;	
	int result;
	struct thread *child;
	struct addrspace *address_child;

//	kprintf("\n");

//	int temp = free_core_page_number();
//        kprintf("\n number of core map pages is %d\n", temp);


	child_tf = kmalloc(sizeof(struct trapframe));
        if(child_tf == NULL){
		kfree(child_tf);
                return ENOMEM;
        }



	result = as_copy(curthread->t_addrspace, &address_child);
	if(result){
		
		kfree(child_tf);
		return ENOMEM; 
	}
	
	
	*child_tf = *tf;

//	splraise(IPL_NONE, IPL_HIGH);
	

//	kprintf("\n");


//	I'll just make thread_fork do the rest of the work because it's hard to synch this stuff
	result = thread_fork("Brat Thread", enter, (struct trapframe *) child_tf, (unsigned long) address_child, &child);
	if(result){
		kfree(child_tf);
		return ENOMEM;
	}

//	kprintf("\n");

//	proc_create(child, child->pid, curthread->pid);

//	kprintf("\n");

	*retval = child->pid;

//	spllower(IPL_HIGH, IPL_NONE);

//	kprintf("\n");

	return 0;
}


void enter(void* tf, unsigned long addrs){

	struct trapframe new_tf;
	struct addrspace *address;

	new_tf = *(struct trapframe *)tf;
	new_tf.tf_v0 = 0;
	new_tf.tf_a3 = 0;
	new_tf.tf_epc += 4;

	address = (struct addrspace *) addrs;
	curthread->t_addrspace = address;
	as_activate(address);	
		
	mips_usermode(&new_tf);
}

void sys_exit(int exitcode){
	int i;
	if(curthread->pid != 1 && proc_tbl[proc_tbl[curthread->pid]->par_pid]->exit == false){
//		if(proc_tbl[curthread->pid] == NULL){
//			kprintf("the process with pid %d\n", curthread->pid);
//		}
		proc_tbl[curthread->pid]->exit_code = _MKWAIT_EXIT(exitcode);
		proc_tbl[curthread->pid]->exit = true;
		V(proc_tbl[curthread->pid]->sem);
	}	
	else{
		sem_destroy(proc_tbl[curthread->pid]->sem);
		
		for(i = 0; i < FD_LIMIT; i++)
		{
			sys_close(i);
			
		}
		kfree(proc_tbl[curthread->pid]);
		proc_tbl[curthread->pid] = NULL;
		if(curthread->pid == 1){
			V(menu_semaphore);
		}
	}

	thread_exit();
}


pid_t find_pid(void){
	int count;
	for(count = 2; count < PID_LIMIT; count++){
		if(proc_tbl[count] == NULL){
			return (pid_t)count;
		}

	}
	return -2;
}

void proc_create(struct thread *td, pid_t id, pid_t parent_pid){
	struct process *proc;
	struct semaphore *sema;
	proc = (struct process *) kmalloc(sizeof(struct process));
	proc->exit = false;
	proc->exit_code = 0;
	proc->thd = td;
	sema = sem_create("child", 0);
	proc->sem = sema;
	proc->par_pid = parent_pid;
	proc_tbl[id] = proc;
}

int sys_sbrk(intptr_t amount, int *ret){

	
	struct addrspace *addr = curthread->t_addrspace;
	vaddr_t heap_end = addr->heap_end;
	vaddr_t heap_start = addr->heap_start;
	vaddr_t new_heap_end;

	if(amount == 0){
		*ret = heap_end;
		return 0;
	}

	if((amount < 0) && ((int)(heap_end - heap_start) < (0 - amount))){
		return EINVAL;
	}

	if(heap_end + amount < heap_start){
		*ret = -1;
		return EINVAL;
	}

	new_heap_end = heap_end + amount;

	if(new_heap_end >= addr->stack_region_start->virtualAddress){
		*ret = -1;
		return ENOMEM;
	}

	struct page* last_heap_page = addr->heap_region_start;
	while(last_heap_page->next != NULL){
		last_heap_page = last_heap_page->next;
	}

	if(new_heap_end > (last_heap_page->virtualAddress + PAGE_SIZE)){

		if((amount/PAGE_SIZE) > free_core_page_number()){
			*ret = -1;
			return ENOMEM;
		}

		while(new_heap_end > (last_heap_page->virtualAddress + PAGE_SIZE)){
			last_heap_page->next = (struct page*)kmalloc(sizeof(struct page));
			last_heap_page->next->virtualAddress = last_heap_page->virtualAddress + PAGE_SIZE;

			last_heap_page->next->write_flag = last_heap_page->write_flag;
			last_heap_page->next->read_flag = last_heap_page->read_flag;
			last_heap_page->next->exec_flag = last_heap_page->exec_flag;
						
			last_heap_page = last_heap_page->next;
			
			last_heap_page->physicalAddress = page_alloc(1);
	
			last_heap_page->next = NULL;
		}
		*ret = heap_end;
		curthread->t_addrspace->heap_end += amount;
	}
	else if(new_heap_end < last_heap_page->virtualAddress){

//		if((int)(heap_end - heap_start) < (0 - amount)){
//			return EINVAL;
//		}		

		while(new_heap_end < last_heap_page->virtualAddress){
			last_heap_page = addr->heap_region_start;
			while(last_heap_page->next->next != NULL){
				last_heap_page = last_heap_page->next;
			}
			freeoneaddress(last_heap_page->next->physicalAddress);
			kfree(last_heap_page->next);
			last_heap_page->next = NULL;
		}
		*ret = heap_end;
		curthread->t_addrspace->heap_end -= amount;
	}
	else if((new_heap_end > last_heap_page->virtualAddress) && (new_heap_end < (last_heap_page->virtualAddress + PAGE_SIZE))){
		*ret = heap_end;
		curthread->t_addrspace->heap_end += amount;
	}

	return 0;
}
