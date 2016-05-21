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

/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than this function does.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <lib.h>
#include <thread.h>
#include <current.h>
#include <addrspace.h>
#include <vm.h>
#include <vfs.h>
#include <syscall.h>
#include <test.h>
#include <kern/proccalls.h>
#include <synch.h>
#include <limits.h>
#include <copyinout.h>
/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
int
runprogram(char *progname, char **args, int numargs)
{
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result, r, index, bytenum;
//	char **temparg;
	

	struct vnode *in_vn;
        char *in = kstrdup("con:");
        struct vnode *out_vn;
        char *out = kstrdup("con:");
        struct vnode *er_vn;
        char *er = kstrdup("con:");


//	if(proclock == NULL){
//		spinlock_init(proclock);
//	}


//	kprintf("entered runprongram\n");

	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, 0, &v);
	if (result) {
		return result;
	}

	/* We should be a new thread. */
	KASSERT(curthread->t_addrspace == NULL);

	/* Create a new address space. */
	curthread->t_addrspace = as_create();
	if (curthread->t_addrspace==NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	/* Activate it. */
	as_activate(curthread->t_addrspace);
//	kprintf("about to load\n");
	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* thread_exit destroys curthread->t_addrspace */
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);
//	kprintf("done\n");
	/* Define the user stack in the address space */
	result = as_define_stack(curthread->t_addrspace, &stackptr);
	if (result) {
		/* thread_exit destroys curthread->t_addrspace */
		return result;
	}
//	kprintf("opening?\n");


//	if(curthread->fd_tbl[0] == NULL){
		r = vfs_open(in, O_RDONLY, 0, &in_vn);
		if(r) {
			return EINVAL;
		}
		curthread->fd_tbl[0] = (struct filehandle *)kmalloc(sizeof(struct filehandle));
		curthread->fd_tbl[0]->vn = in_vn;
		curthread->fd_tbl[0]->openflags = O_RDONLY;
		curthread->fd_tbl[0]->offset = 0;
		curthread->fd_tbl[0]->refcnt = 1;
//		curthread->fd_tbl[0]->lk = lock_create("input");

		r = vfs_open(out, O_WRONLY, 0, &out_vn);
		if(r) {
			return EINVAL;
		}

		curthread->fd_tbl[1] = (struct filehandle *)kmalloc(sizeof(struct filehandle));
		curthread->fd_tbl[1]->vn = out_vn;
		curthread->fd_tbl[1]->openflags = O_WRONLY;
		curthread->fd_tbl[1]->offset = 0;
		curthread->fd_tbl[1]->refcnt = 1;
//		curthread->fd_tbl[1]->lk = lock_create("output");

		r = vfs_open(er, O_WRONLY, 0, &er_vn);
		if(r) {
			return EINVAL;
		}

		curthread->fd_tbl[2] = (struct filehandle *)kmalloc(sizeof(struct filehandle));
		curthread->fd_tbl[2]->vn = er_vn;
		curthread->fd_tbl[2]->openflags = O_WRONLY;
		curthread->fd_tbl[2]->offset = 0;
		curthread->fd_tbl[2]->refcnt = 1;
//		curthread->fd_tbl[2]->lk = lock_create("err");
//	}




	struct process *proc;
	struct semaphore *sema;
	proc = (struct process *) kmalloc(sizeof(struct process));
	curthread->pid = 1;
        proc->exit = false;
        proc->exit_code = 0;
        proc->thd = curthread;
        sema = sem_create("first_thread", 0);
        proc->sem = sema;
        proc->par_pid = 0;
        proc_tbl[1] = proc;


	if(numargs == 1){
		args[1] = NULL;
	}
	else{
		args[numargs] = NULL;
	}

	
	int arglen;
//	int totallength;


//	temparg = (char **)kmalloc(sizeof(char*) * numargs);
		
	for(index = 0, bytenum = 0; args[index] != NULL; index++){
		arglen = strlen(args[index])+1; //adding the 1 so I can have it be null terminated, as said on piazza
		bytenum = arglen; //arglen is the size of the arg, with no 0s
		bytenum = (4 - (bytenum % 4));//bytenum is the number of 0s needed to pad the argument on the stack
		char* onearg = kmalloc(sizeof(bytenum+arglen));//onearg should be bigger then the argument for the null terminating 0s
		onearg = kstrdup(args[index]);
		for(int i = 0; i < (bytenum + arglen); i++){
			if(i < arglen){
				onearg[i] = args[index][i];
			}
			else{
				onearg[i] = '\0';
			}
		} //onearg is now the entire argument, with padded 0s, that will be put on the stack
		result = strlen(onearg);
		stackptr -= (bytenum + arglen);
		result = copyout((const void *)onearg, (userptr_t)stackptr, (size_t)(bytenum+arglen)); //puts the current arg, with padding, onto the stack
		if(result){return result;}
//		totallength = totallength + bytenum + argnum;
//		temparg[index] = onearg;
		args[index] = (char *)stackptr;
		kfree(onearg);
	}

	
	result = sizeof(char*);
	for(index = numargs; index >= 0; index--){//index < 0; index--){
		stackptr = stackptr - 4;
		if(args[index] != NULL){	
			result = copyout((const void *)(args+index), (userptr_t)stackptr, (sizeof(char *)));	
			if(result) {
				return result;
			}
		}
	}	

		//at this point temparg should be a string array with each padded 
		//argument at the appropriate index, name at 0 and so on
		//and totallength is the length of all padded arguments
/*	char* karg = (char *)kmalloc(sizeof(char) * totallength); //this is making an array to hold one arg char at each index
	
	//this loop should place each arg character from the temparg string array in the correct order into the char array
	int argindex = 0;
	
	for(index  = 0; index < numargs; index++){
		for(int i = 0; i < strlen(temparg[index]); i++){
			karg[argindex] = temparg[index][i];
			argindex++;	
		}
	}
	//karg hopefully has all of the chars, in the correct order, that make up the arguments, including the padding 0s
	
	stackptr = stackptr - totallength;
	result = copyout((const void *)temparg

*/	
	
	
	

	

	/* Warp to user mode. */
	enter_new_process(numargs /*argc*/, (userptr_t)stackptr /*userspace addr of argv*/,
			  stackptr, entrypoint);
	
	/* enter_new_process does not return. */
	panic("enter_new_process returned\n");
	return EINVAL;
}

